#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO Reduce code repetition on parsing functions: TryParseUnaryPrimitive,
// TryParseBinaryPrimitive, TryParseIfExpr, TryParseAndExpr, TryParseOrExpr.

// TODO Switch emitted code to intel syntax.

// TODO Parsing is too sensitive to whitespace. Make it more robust.

using namespace std;

using TBindings = unordered_map<string, string>;
using TEnvironment = unordered_map<string, int>;
using TUnaryPrimitiveEmitter = string (*)(int, TEnvironment, string);
using TBinaryPrimitiveEmitter = string (*)(int, TEnvironment, string, string);

string Exec(const char *cmd);

bool IsFixNum(string token);
bool IsBool(string token);
bool IsNull(string token);
bool IsChar(string token);
bool IsImmediate(string token);
bool TryParseUnaryPrimitive(string expr, string *outPrimitiveName = nullptr,
                            string *outArg = nullptr);
bool TryParseBinaryPrimitive(string expr, string *outPrimitiveName = nullptr,
                             vector<string> *outArgs = nullptr);
bool TryParseSubExpr(string expr, size_t subExprStart,
                     string *subExpr = nullptr, size_t *subExprEnd = nullptr);
bool TryParseVariableNumOfSubExpr(string expr, size_t startIdx,
                                  vector<string> *outSubExprs,
                                  int expectedNumSubExprs = -1);
bool TryParseIfExpr(string expr, vector<string> *outIfParts = nullptr);
bool TryParseAndExpr(string expr, vector<string> *outAndArgs = nullptr);
bool TryParseOrExpr(string expr, vector<string> *outOrArgs = nullptr);
bool TryParseLetExpr(string expr, TBindings *outBindings = nullptr,
                     string *outLetBody = nullptr);
bool IsExpr(string expr);

char TokenToChar(string token);
int ImmediateRep(string token);

string EmitFxAddImmediate(int stackIdx, TEnvironment env, string fxAddArg,
                          string fxAddImmediate);
string EmitFxAdd1(int stackIdx, TEnvironment env, string fxAdd1Arg);
string EmitFxSub1(int stackIdx, TEnvironment env, string fxSub1Arg);
string EmitFixNumToChar(int stackIdx, TEnvironment env, string fixNumToCharArg);
string EmitCharToFixNum(int stackIdx, TEnvironment env, string fixNumToCharArg);
string EmitIsFixNum(int stackIdx, TEnvironment env, string isFixNumArg);
string EmitIsFxZero(int stackIdx, TEnvironment env, string isFxZeroArg);
string EmitIsNull(int stackIdx, TEnvironment env, string isNullArg);
string EmitIsBoolean(int stackIdx, TEnvironment env, string isBooleanArg);
string EmitIsChar(int stackIdx, TEnvironment env, string isCharArg);
string EmitNot(int stackIdx, TEnvironment env, string notArg);
string EmitFxLogNot(int stackIdx, TEnvironment env, string fxLogNotArg);
string EmitFxAdd(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxSub(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxMul(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxLogOr(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxLogAnd(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxCmp(int stackIdx, TEnvironment env, string lhs, string rhs,
                 string setcc);
string EmitFxEq(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxLT(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxLE(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxGT(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitFxGE(int stackIdx, TEnvironment env, string lhs, string rhs);
string EmitIfExpr(int stackIdx, TEnvironment env, string cond, string conseq,
                  string alt);
string EmitAndExpr(int stackIdx, TEnvironment env,
                   const vector<string> &andArgs);
string EmitExpr(int stackIdx, TEnvironment env, string expr);

string UniqueLabel() {
    static unsigned int count = 0;
    return "L_" + to_string(count++);
}

string EmitProgram(string programSource);

string Exec(const char *cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result.substr(0, result.size() - 1);
}

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const unsigned int FxMaskNeg = 0xFFFFFFFC;
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;
const unsigned int BoolMask = 0x3F;
const unsigned int BoolTag = 0x2F;
const unsigned int BoolBit = 6;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
const unsigned int CharMask = 0xFF;
const unsigned int CharTag = 0x0F;

const unsigned int WordSize = 4;

const unsigned int FixNumBits = WordSize * 8 - FxShift;
const int FxLower = -pow(2, FixNumBits - 1);
const int FxUpper = pow(2, FixNumBits - 1) - 1;

bool IsFixNum(string token) {
    try {
        auto intVal = stoi(token);
        return (FxLower <= intVal) && (intVal <= FxUpper);
    } catch (exception &) {
        return false;
    }
}

bool IsBool(string token) { return token == "#f" || token == "#t"; }

bool IsNull(string token) { return token == "()"; }

bool IsChar(string token) {
    if (token == "#\\~") {
        int dummy = 10;
    }

    if (token.size() < 3) {
        return false;
    }

    if (token[0] != '#' || token[1] != '\\') {
        return false;
    }

    if (token == "#\\space" || token == "#\\newline" || token == "#\\tab" ||
        token == "#\\return") {
        return true;
    }

    if (token.size() > 3) {
        return false;
    }

    const string specialChars = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
    return isalnum(token[2]) || specialChars.find(token[2]) != string::npos;
}

bool IsImmediate(string token) {
    return IsBool(token) || IsNull(token) || IsChar(token) || IsFixNum(token);
}

bool IsVarName(string token) {
    for (auto c : token) {
        if (!isalpha(c)) {
            return false;
        }
    }

    return true;
}

bool TryParseUnaryPrimitive(string expr, string *outPrimitiveName,
                            string *outArg) {
    if (expr.size() < 3) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    static vector<string> unaryPrimitiveNames{
        "fxadd1",  "fxsub1",  "fixnum->char", "char->fixnum",
        "fixnum?", "fxzero?", "null?",        "boolean?",
        "char?",   "not",     "fxlognot"};

    string primitiveName = "";
    size_t idx;

    for (idx = 1; idx < (expr.size() - 1) && !isspace(expr[idx]); ++idx) {
        primitiveName = primitiveName + expr[idx];
    }

    if (find(unaryPrimitiveNames.begin(), unaryPrimitiveNames.end(),
             primitiveName) == unaryPrimitiveNames.end()) {
        return false;
    }

    if (outPrimitiveName != nullptr) {
        *outPrimitiveName = primitiveName;
    }

    // No argument provided.
    if (!isspace(expr[idx])) {
        return false;
    }

    for (; idx < (expr.size() - 1) && isspace(expr[idx]); ++idx) {
    }

    string arg = expr.substr(idx, expr.size() - 1 - idx);

    if (outArg != nullptr) {
        *outArg = arg;
    }

    return IsExpr(arg);
}

bool TryParseBinaryPrimitive(string expr, string *outPrimitiveName,
                             vector<string> *outArgs) {
    if (expr.size() < 3) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    static vector<string> binaryPrimitiveNames{
        "fx+", "fx-", "fx*",  "fxlogor", "fxlogand",
        "fx=", "fx<", "fx<=", "fx>",     "fx>="};

    string primitiveName = "";
    size_t idx;

    for (idx = 1; idx < (expr.size() - 1) && !isspace(expr[idx]); ++idx) {
        primitiveName = primitiveName + expr[idx];
    }

    if (find(binaryPrimitiveNames.begin(), binaryPrimitiveNames.end(),
             primitiveName) == binaryPrimitiveNames.end()) {
        return false;
    }

    if (outPrimitiveName != nullptr) {
        *outPrimitiveName = primitiveName;
    }

    return TryParseVariableNumOfSubExpr(expr, idx, outArgs, 2);
}

bool TryParseSubExpr(string expr, size_t subExprStart, string *outSubExpr,
                     size_t *outSubExprEnd) {
    // The (- 1) accounts for the closing ) of expr.
    assert(subExprStart < expr.size() - 1);

    size_t idx = subExprStart;
    string subExpr;

    if (expr[idx] != '(') {
        // Immediate.
        for (; idx < (expr.size() - 1) && !isspace(expr[idx]); ++idx) {
        }
    } else {
        // Parenthesized expression.
        int numOpenParen = 1;
        ++idx;

        for (; idx < (expr.size() - 1) && numOpenParen > 0; ++idx) {
            if (expr[idx] == '(') {
                ++numOpenParen;
            }

            if (expr[idx] == ')') {
                --numOpenParen;
            }
        }
    }

    subExpr = expr.substr(subExprStart, idx - subExprStart);

    if (outSubExpr != nullptr) {
        *outSubExpr = expr.substr(subExprStart, idx - subExprStart);
    }

    if (outSubExprEnd != nullptr) {
        *outSubExprEnd = idx;
    }

    return IsExpr(subExpr);
}

bool skipSpaceAndCheckIfEndOfExpr(string expr, size_t *idx) {
    assert(*idx < expr.size());

    for (; *idx < (expr.size() - 1) && isspace(expr[*idx]); ++*idx) {
    }

    return (*idx == (expr.size() - 1));
}

bool TryParseVariableNumOfSubExpr(string expr, size_t startIdx,
                                  vector<string> *outSubExprs,
                                  int expectedNumSubExprs) {
    if (outSubExprs != nullptr) {
        outSubExprs->clear();
    }

    size_t idx = startIdx;
    int numSubExprs = 0;

    while (!skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        string *argPtr = nullptr;

        if (outSubExprs != nullptr) {
            outSubExprs->push_back("");
            argPtr = &outSubExprs->data()[outSubExprs->size() - 1];
        }

        if (!TryParseSubExpr(expr, idx, argPtr, &idx)) {
            return false;
        }

        ++numSubExprs;
    }

    return (expectedNumSubExprs == -1) || (expectedNumSubExprs == numSubExprs);
}

bool TryParseIfExpr(string expr, vector<string> *outIfParts) {
    if (expr.size() < 4) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'i' || expr[2] != 'f') {
        return false;
    }

    return TryParseVariableNumOfSubExpr(expr, 3, outIfParts, 3);
}

bool TryParseAndExpr(string expr, vector<string> *outAndArgs) {
    if (expr.size() < 5) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'a' || expr[2] != 'n' || expr[3] != 'd') {
        return false;
    }

    return TryParseVariableNumOfSubExpr(expr, 4, outAndArgs);
}

bool TryParseOrExpr(string expr, vector<string> *outOrArgs) {
    if (expr.size() < 4) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'o' || expr[2] != 'r') {
        return false;
    }

    return TryParseVariableNumOfSubExpr(expr, 3, outOrArgs);
}

bool TryParseLetExpr(string expr, TBindings *outBindings, string *outLetBody) {
    if (expr.size() < 5) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'l' || expr[2] != 'e' || expr[3] != 't') {
        return false;
    }

    size_t idx = 4;
    if (skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    // Parse variable definitions.
    if (expr[idx] != '(') {
        return false;
    }

    while (true) {
        ++idx;

        if (expr[idx] == ')') {
            break;
        }

        if (skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
            return false;
        }

        if (expr[idx] != '[') {
            return false;
        }

        ++idx;

        auto varEnd = expr.find(' ', idx);

        if (varEnd == string::npos) {
            return false;
        }

        auto varName = expr.substr(idx, varEnd - idx);
        idx = varEnd;

        if (skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
            return false;
        }

        auto defStart = idx;

        // Bracketed expression.
        int numOpenBrac = 1;
        ++idx;

        for (; idx < expr.size() && numOpenBrac > 0; ++idx) {
            if (expr[idx] == '[') {
                ++numOpenBrac;
            }

            if (expr[idx] == ']') {
                --numOpenBrac;
            }
        }

        if (idx == expr.size()) {
            return false;
        }

        auto varDef = expr.substr(defStart, idx - defStart - 1);

        if (!IsExpr(varDef)) {
            return false;
        }

        if (outBindings != nullptr) {
            outBindings->insert({varName, varDef});
        }

        --idx;
    }

    ++idx;

    if (skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    auto letBody = expr.substr(idx, expr.size() - 1 - idx);

    if (outLetBody != nullptr) {
        *outLetBody = letBody;
    }

    return IsExpr(letBody);
}

bool IsExpr(string expr) {
    return IsImmediate(expr) || IsVarName(expr) ||
           TryParseUnaryPrimitive(expr) || TryParseBinaryPrimitive(expr) ||
           TryParseIfExpr(expr) || TryParseAndExpr(expr) ||
           TryParseOrExpr(expr) || TryParseLetExpr(expr);
}

char TokenToChar(string token) {
    assert(IsChar(token));

    if (token == "#\\space") {
        return ' ';
    }

    if (token == "#\\newline") {
        return '\n';
    }

    if (token == "#\\tab") {
        return '\t';
    }

    if (token == "#\\return") {
        return '\r';
    }

    return token[2];
}

int ImmediateRep(string token) {
    assert(IsImmediate(token));

    if (IsNull(token)) {
        return Null;
    }

    if (IsBool(token)) {
        return token == "#f" ? BoolF : BoolT;
    }

    if (IsChar(token)) {
        return (static_cast<int>(TokenToChar(token)) << CharShift) | CharTag;
    }

    // Else, must be fixnum.
    return (stoi(token) << FxShift) | FxTag;
}

string EmitVarRef(TEnvironment env, string expr) {
    auto stackIdx = env.at(expr);
    return "    movl " + to_string(stackIdx) + "(%rsp), %eax\n";
}

string EmitFxAddImmediate(int stackIdx, TEnvironment env, string fxAddArg,
                          string fxAddImmediate) {
    assert(IsImmediate(fxAddImmediate));

    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream 
      << EmitExpr(stackIdx, env, fxAddArg)
      << "    addl $" << ImmediateRep(fxAddImmediate) << ", %eax\n";
  // clang-format on 

  return exprEmissionStream.str();

}

string EmitFxAdd1(int stackIdx, TEnvironment env, string fxAdd1Arg) {
    return EmitFxAddImmediate(stackIdx, env, fxAdd1Arg, "1");
}

string EmitFxSub1(int stackIdx, TEnvironment env, string fxSub1Arg) {
    return EmitFxAddImmediate(stackIdx, env, fxSub1Arg, "-1");
}

string EmitFixNumToChar(int stackIdx, TEnvironment env, string fixNumToCharArg) {
  ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, fixNumToCharArg)
      << "    shll $" << (CharShift - FxShift) << ", %eax\n"
      << "    orl $" << CharTag << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitCharToFixNum(int stackIdx, TEnvironment env,
                        string charToFixNumArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, charToFixNumArg)
      << "    shrl $" << (CharShift - FxShift) << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsFixNum(int stackIdx, TEnvironment env, string isFixNumArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isFixNumArg)
      << "    and $" << FxMask << ", %al\n"
      << "    cmp $" << FxTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsFxZero(int stackIdx, TEnvironment env, string isFxZeroArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isFxZeroArg)
      << "    cmpl $0, %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsNull(int stackIdx, TEnvironment env, string isNullArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isNullArg)
      << "    cmpl $" << Null << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsBoolean(int stackIdx, TEnvironment env, string isBooleanArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isBooleanArg)
      << "    and $" << BoolMask << ", %al\n"
      << "    cmp $" << BoolTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsChar(int stackIdx, TEnvironment env, string isCharArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isCharArg)
      << "    and $" << CharMask << ", %al\n"
      << "    cmp $" << CharTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitNot(int stackIdx, TEnvironment env, string notArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, notArg)
      << "    cmpl $" << BoolF << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogNot(int stackIdx, TEnvironment env, string fxLogNotArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, fxLogNotArg)
      << "    xor $" << FxMaskNeg << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxAdd(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    addl " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxSub(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, rhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, lhs)
      << "    subl " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxMul(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    sarl $" << FxShift << ", %eax\n"
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    imul " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogOr(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    or " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogAnd(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    and " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxCmp(int stackIdx, TEnvironment env, string lhs, string rhs,
                 string setcc) {
    ostringstream exprEmissionStream;
    // clang-format off
    exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    cmpl  %eax, " << stackIdx << "(%rsp)\n"
      << "    " << setcc << " %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxEq(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "sete");
}

string EmitFxLT(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setl");
}

string EmitFxLE(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setle");
}

string EmitFxGT(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setg");
}

string EmitFxGE(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setge");
}

string EmitIfExpr(int stackIdx, TEnvironment env, string cond, string conseq,
                  string alt) {
    string altLabel = UniqueLabel();
    string endLabel = UniqueLabel();

    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, cond)
      << "    cmp $" << BoolF << ", %al\n"
      << "    je " << altLabel << "\n"
      << EmitExpr(stackIdx, env, conseq)
      << "    jmp " << endLabel << "\n"
      << altLabel << ":\n"
      << EmitExpr(stackIdx, env, alt)
      << endLabel << ":\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitLogicalExpr(int stackIdx, TEnvironment env,
                       const vector<string> &args, bool isAnd) {
    ostringstream exprEmissionStream;

    if (args.size() == 0) {
        exprEmissionStream << EmitExpr(stackIdx, env, "#t");
    } else if (args.size() == 1) {
        exprEmissionStream << EmitExpr(stackIdx, env, args[0]);
    } else {
        ostringstream newExpr;
        newExpr << "(" << (isAnd ? "and" : "or");

        for (int i = 1; i < args.size(); ++i) {
            newExpr << " " << args[i];
        }

        newExpr << ")";

        if (isAnd) {
            exprEmissionStream
                << EmitIfExpr(stackIdx, env, args[0], newExpr.str(), "#f");
        } else {
            exprEmissionStream
                << EmitIfExpr(stackIdx, env, args[0], "#t", newExpr.str());
        }
    }

    return exprEmissionStream.str();
}

string EmitAndExpr(int stackIdx, TEnvironment env,
                   const vector<string> &andArgs) {
    return EmitLogicalExpr(stackIdx, env, andArgs, true);
}

string EmitOrExpr(int stackIdx, TEnvironment env,
                  const vector<string> &orArgs) {
    return EmitLogicalExpr(stackIdx, env, orArgs, false);
}

string EmitStackSave(int stackIdx) {
    return "    movl %eax, " + to_string(stackIdx) + "(%rsp)\n";
}

string EmitLetExpr(int stackIdx, TEnvironment env, const TBindings &bindings,
                   string letBody) {
    ostringstream exprEmissionStream;
    int si = stackIdx;
    TEnvironment envExtension;

    for (auto b : bindings) {
        // clang-format off
        exprEmissionStream
            << EmitExpr(si, env, b.second)
            << EmitStackSave(si);
        // clang-format on
        envExtension.insert({b.first, si});
        si -= WordSize;
    }

    for (auto v : envExtension) {
        env[v.first] = v.second;
    }

    exprEmissionStream << EmitExpr(si, env, letBody);

    return exprEmissionStream.str();
}

string EmitExpr(int stackIdx, TEnvironment env, string expr) {
    assert(IsExpr(expr));

    if (IsImmediate(expr)) {
        ostringstream exprEmissionStream;
        // clang-format off
    exprEmissionStream 
        << "    movl $" << ImmediateRep(expr) << ", %eax\n";
        // clang-format on

        return exprEmissionStream.str();
    }

    if (IsVarName(expr)) {
        return EmitVarRef(env, expr);
    }

    string primitiveName;
    string arg;

    if (TryParseUnaryPrimitive(expr, &primitiveName, &arg)) {
        static unordered_map<string, TUnaryPrimitiveEmitter> unaryEmitters{
            {"fxadd1", EmitFxAdd1},
            {"fxsub1", EmitFxSub1},
            {"fixnum->char", EmitFixNumToChar},
            {"char->fixnum", EmitCharToFixNum},
            {"fixnum?", EmitIsFixNum},
            {"fxzero?", EmitIsFxZero},
            {"null?", EmitIsNull},
            {"boolean?", EmitIsBoolean},
            {"char?", EmitIsChar},
            {"not", EmitNot},
            {"fxlognot", EmitFxLogNot}};
        return unaryEmitters[primitiveName](stackIdx, env, arg);
    }

    vector<string> args;

    if (TryParseBinaryPrimitive(expr, &primitiveName, &args)) {
        assert(args.size() == 2);
        static unordered_map<string, TBinaryPrimitiveEmitter> binaryEmitters{
            {"fx+", EmitFxAdd},         {"fx-", EmitFxSub},
            {"fx*", EmitFxMul},         {"fxlogor", EmitFxLogOr},
            {"fxlogand", EmitFxLogAnd}, {"fx=", EmitFxEq},
            {"fx<", EmitFxLT},          {"fx<=", EmitFxLE},
            {"fx>", EmitFxGT},          {"fx>=", EmitFxGE}};
        assert(binaryEmitters[primitiveName] != nullptr);
        return binaryEmitters[primitiveName](stackIdx, env, args[0], args[1]);
    }

    vector<string> ifParts;

    if (TryParseIfExpr(expr, &ifParts)) {
        assert(ifParts.size() == 3);
        return EmitIfExpr(stackIdx, env, ifParts[0], ifParts[1], ifParts[2]);
    }

    vector<string> andArgs;

    if (TryParseAndExpr(expr, &andArgs)) {
        return EmitAndExpr(stackIdx, env, andArgs);
    }

    vector<string> orArgs;

    if (TryParseOrExpr(expr, &orArgs)) {
        return EmitOrExpr(stackIdx, env, orArgs);
    }

    TBindings bindings;
    string letBody;

    if (TryParseLetExpr(expr, &bindings, &letBody)) {
        return EmitLetExpr(stackIdx, env, bindings, letBody);
    }

    assert(false);
}

string EmitProgram(string programSource) {
    ostringstream programEmissionStream;
    // clang-format off
  programEmissionStream 
      << "    .text\n"
      << "    .globl scheme_entry\n"
      << "    .type scheme_entry, @function\n"
      << "scheme_entry:\n"
      << "    movq %rsp, %rcx\n"
      << "    movq %rdi, %rsp\n"
      << EmitExpr(-4, TEnvironment(), programSource)
      << "    movq %rcx, %rsp\n"
      << "    ret\n";
    // clang-format on

    return programEmissionStream.str();
}

int main(int argc, char *argv[]) {
    vector<string> testFilePaths{
        "/home/ergawy/repos/inc-compiler/src/tests-1.1-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.2-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.3-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.4-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.5-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.6-req.scm"};

    int testCaseCounter = 1;
    int failedTestCaseCounter = 0;

    for (const auto &tfp : testFilePaths) {
        ifstream testFile(tfp);

        if (!testFile.is_open()) {
            cerr << "Cannot open test file.\n";
            return 1;
        }

        const size_t MAX_LINE_SIZE = 100;
        char ignoredLine[MAX_LINE_SIZE];

        while (true) {
            char nextChar;
            testFile >> nextChar;

            if (testFile.eof()) {
                break;
            }

            // Test suit header
            if (nextChar == '(') {
                testFile.getline(ignoredLine, MAX_LINE_SIZE);
                continue;
            }

            if (nextChar == ')') {
                continue;
            }

            // Comment.
            if (nextChar == ';') {
                testFile.getline(ignoredLine, MAX_LINE_SIZE);
                continue;
            }

            //
            // Parse test case.
            //

            // Parse program source.
            ostringstream programSourceOutputStream;

            string nextProgramSubString;
            testFile >> nextProgramSubString;
            programSourceOutputStream << nextProgramSubString;

            while (true) {
                testFile >> nextProgramSubString;

                if (nextProgramSubString == "=>") {
                    break;
                }

                programSourceOutputStream << " " << nextProgramSubString;
            }

            string programSource = programSourceOutputStream.str();

            // Parse expected program output.
            ostringstream expectedResultOutputStream;

            while (true) {
                string nextResultSubString;
                testFile >> nextResultSubString;
                bool lastSubString = nextResultSubString.back() == ']';
                expectedResultOutputStream
                    << (lastSubString ? nextResultSubString.substr(
                                            0, nextResultSubString.size() - 1)
                                      : nextResultSubString);

                if (lastSubString) {
                    break;
                }
            }

            string expectedResult = expectedResultOutputStream.str();
            expectedResult =
                expectedResult.substr(1, expectedResult.size() - 4);

            auto programAsm = EmitProgram(programSource);

            string testId = "test-" + to_string(testCaseCounter);
            ofstream programAsmOutputStream(testId + ".s");

            if (!programAsmOutputStream.is_open()) {
                cerr << "Couldn't dumpt ASM to output file.";
                return 1;
            }

            programAsmOutputStream << programAsm;
            programAsmOutputStream.close();

            Exec(("gcc /home/ergawy/repos/sil-compiler/runtime.c " + testId +
                  ".s -o " + testId + ".out")
                     .c_str());
            auto actualResult = Exec(("./" + testId + ".out").c_str());

            cout << "[TEST " << testCaseCounter << "]\n";
            cout << programSource << "\n";

            cout << "\t Expected: " << expectedResult << "\n"
                 << "\t Actual  : " << actualResult << "\n";

            if (actualResult == expectedResult) {
                cout << "\033[1;32mOK\033[0m\n\n";
            } else {
                cout << "\033[1;31mFAILED\033[0m\n\n";
                ++failedTestCaseCounter;
            }

            ++testCaseCounter;
        }
    }

    if (failedTestCaseCounter > 0) {
        cout << "\n\033[1;31mFailed/Total: " << failedTestCaseCounter << "/"
             << (testCaseCounter - 1) << "\033[0m\n";
    }

    return 0;
}
