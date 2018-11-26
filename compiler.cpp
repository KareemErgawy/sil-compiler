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

// TODO Switch emitted code to inter syntax.

std::string Exec(const char *cmd);

bool IsFixNum(std::string token);
bool IsBool(std::string token);
bool IsNull(std::string token);
bool IsChar(std::string token);
bool IsImmediate(std::string token);
bool TryParseUnaryPrimitive(std::string expr,
                            std::string *outPrimitiveName = nullptr,
                            std::string *outArg = nullptr);
bool TryParseBinaryPrimitive(std::string expr,
                             std::string *outPrimitiveName = nullptr,
                             std::vector<std::string> *outArgs = nullptr);
bool TryParseSubExpr(std::string expr, size_t subExprStart,
                     std::string *subExpr = nullptr,
                     size_t *subExprEnd = nullptr);
bool TryParseVariableNumOfSubExpr(std::string expr, size_t startIdx,
                                  std::vector<std::string> *outSubExprs,
                                  int expectedNumSubExprs = -1);
bool TryParseIfExpr(std::string expr,
                    std::vector<std::string> *outIfParts = nullptr);
bool TryParseAndExpr(std::string expr,
                     std::vector<std::string> *outAndArgs = nullptr);
bool TryParseOrExpr(std::string expr,
                    std::vector<std::string> *outOrArgs = nullptr);
bool IsExpr(std::string expr);

char TokenToChar(std::string token);
int ImmediateRep(std::string token);

using TUnaryPrimitiveEmitter = std::string (*)(int, std::string);
using TBinaryPrimitiveEmitter = std::string (*)(int, std::string, std::string);

std::string EmitFxAddImmediate(int stackIdx, std::string fxAddArg,
                               std::string fxAddImmediate);
std::string EmitFxAdd1(int stackIdx, std::string fxAdd1Arg);
std::string EmitFxSub1(int stackIdx, std::string fxSub1Arg);
std::string EmitFixNumToChar(int stackIdx, std::string fixNumToCharArg);
std::string EmitCharToFixNum(int stackIdx, std::string fixNumToCharArg);
std::string EmitIsFixNum(int stackIdx, std::string isFixNumArg);
std::string EmitIsFxZero(int stackIdx, std::string isFxZeroArg);
std::string EmitIsNull(int stackIdx, std::string isNullArg);
std::string EmitIsBoolean(int stackIdx, std::string isBooleanArg);
std::string EmitIsChar(int stackIdx, std::string isCharArg);
std::string EmitNot(int stackIdx, std::string notArg);
std::string EmitFxLogNot(int stackIdx, std::string fxLogNotArg);
std::string EmitFxAdd(int stackIdx, std::string lhs, std::string rhs);
std::string EmitFxSub(int stackIdx, std::string lhs, std::string rhs);
std::string EmitFxMul(int stackIdx, std::string lhs, std::string rhs);
std::string EmitFxLogOr(int stackIdx, std::string lhs, std::string rhs);
std::string EmitFxLogAnd(int stackIdx, std::string lhs, std::string rhs);
std::string EmitIfExpr(int stackIdx, std::string cond, std::string conseq,
                       std::string alt);
std::string EmitAndExpr(int stackIdx, const std::vector<std::string> &andArgs);
std::string EmitExpr(int stackIdx, std::string expr);

std::string UniqueLabel() {
  static unsigned int count = 0;
  return "L_" + std::to_string(count++);
}

std::string EmitProgram(std::string programSource);

std::string Exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
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
const int FxLower = -std::pow(2, FixNumBits - 1);
const int FxUpper = std::pow(2, FixNumBits - 1) - 1;

bool IsFixNum(std::string token) {
  try {
    auto intVal = std::stoi(token);
    return (FxLower <= intVal) && (intVal <= FxUpper);
  } catch (std::exception &) {
    return false;
  }
}

bool IsBool(std::string token) { return token == "#f" || token == "#t"; }

bool IsNull(std::string token) { return token == "()"; }

bool IsChar(std::string token) {
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

  const std::string specialChars = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  return std::isalnum(token[2]) ||
         specialChars.find(token[2]) != std::string::npos;
}

bool IsImmediate(std::string token) {
  return IsBool(token) || IsNull(token) || IsChar(token) || IsFixNum(token);
}

bool TryParseUnaryPrimitive(std::string expr, std::string *outPrimitiveName,
                            std::string *outArg) {
  if (expr.size() < 3) {
    return false;
  }

  if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
    return false;
  }

  static std::vector<std::string> unaryPrimitiveNames{
      "fxadd1",  "fxsub1",  "fixnum->char", "char->fixnum",
      "fixnum?", "fxzero?", "null?",        "boolean?",
      "char?",   "not",     "fxlognot"};

  std::string primitiveName = "";
  size_t idx;

  for (idx = 1; idx < (expr.size() - 1) && !std::isspace(expr[idx]); ++idx) {
    primitiveName = primitiveName + expr[idx];
  }

  if (std::find(unaryPrimitiveNames.begin(), unaryPrimitiveNames.end(),
                primitiveName) == unaryPrimitiveNames.end()) {
    return false;
  }

  if (outPrimitiveName != nullptr) {
    *outPrimitiveName = primitiveName;
  }

  // No argument provided.
  if (!std::isspace(expr[idx])) {
    return false;
  }

  for (; idx < (expr.size() - 1) && std::isspace(expr[idx]); ++idx) {
  }

  std::string arg = expr.substr(idx, expr.size() - 1 - idx);

  if (outArg != nullptr) {
    *outArg = arg;
  }

  return IsExpr(arg);
}

bool TryParseBinaryPrimitive(std::string expr, std::string *outPrimitiveName,
                             std::vector<std::string> *outArgs) {
  if (expr.size() < 3) {
    return false;
  }

  if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
    return false;
  }

  static std::vector<std::string> binaryPrimitiveNames{"fx+", "fx-", "fx*",
                                                       "fxlogor", "fxlogand"};

  std::string primitiveName = "";
  size_t idx;

  for (idx = 1; idx < (expr.size() - 1) && !std::isspace(expr[idx]); ++idx) {
    primitiveName = primitiveName + expr[idx];
  }

  if (std::find(binaryPrimitiveNames.begin(), binaryPrimitiveNames.end(),
                primitiveName) == binaryPrimitiveNames.end()) {
    return false;
  }

  if (outPrimitiveName != nullptr) {
    *outPrimitiveName = primitiveName;
  }

  return TryParseVariableNumOfSubExpr(expr, idx, outArgs, 2);
}

bool TryParseSubExpr(std::string expr, size_t subExprStart,
                     std::string *outSubExpr, size_t *outSubExprEnd) {
  // The (- 1) accounts for the closing ) of expr.
  assert(subExprStart < expr.size() - 1);

  size_t idx = subExprStart;
  std::string subExpr;

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

bool skipSpaceAndCheckIfEndOfExpr(std::string expr, size_t *idx) {
  assert(*idx < expr.size());

  for (; *idx < (expr.size() - 1) && std::isspace(expr[*idx]); ++*idx) {
  }

  return (*idx == (expr.size() - 1));
}

bool TryParseVariableNumOfSubExpr(std::string expr, size_t startIdx,
                                  std::vector<std::string> *outSubExprs,
                                  int expectedNumSubExprs) {
  if (outSubExprs != nullptr) {
    outSubExprs->clear();
  }

  size_t idx = startIdx;
  int numSubExprs = 0;

  while (!skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
    std::string *argPtr = nullptr;

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

bool TryParseIfExpr(std::string expr, std::vector<std::string> *outIfParts) {
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

bool TryParseAndExpr(std::string expr, std::vector<std::string> *outAndArgs) {
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

bool TryParseOrExpr(std::string expr, std::vector<std::string> *outOrArgs) {
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

bool IsExpr(std::string expr) {
  return IsImmediate(expr) || TryParseUnaryPrimitive(expr) ||
         TryParseBinaryPrimitive(expr) || TryParseIfExpr(expr) ||
         TryParseAndExpr(expr) || TryParseOrExpr(expr);
}

char TokenToChar(std::string token) {
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

int ImmediateRep(std::string token) {
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
  return (std::stoi(token) << FxShift) | FxTag;
}

std::string EmitFxAddImmediate(int stackIdx, std::string fxAddArg,
                               std::string fxAddImmediate) {
  assert(IsImmediate(fxAddImmediate));

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream 
      << EmitExpr(stackIdx, fxAddArg)
      << "    addl $" << ImmediateRep(fxAddImmediate) << ", %eax\n";
  // clang-format on 

  return exprEmissionStream.str();

}

std::string EmitFxAdd1(int stackIdx, std::string fxAdd1Arg) {
    return EmitFxAddImmediate(stackIdx, fxAdd1Arg, "1");
}

std::string EmitFxSub1(int stackIdx, std::string fxSub1Arg) {
    return EmitFxAddImmediate(stackIdx, fxSub1Arg, "-1");
}

std::string EmitFixNumToChar(int stackIdx, std::string fixNumToCharArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, fixNumToCharArg)
      << "    shll $" << (CharShift - FxShift) << ", %eax\n"
      << "    orl $" << CharTag << ", %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitCharToFixNum(int stackIdx, std::string charToFixNumArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, charToFixNumArg)
      << "    shrl $" << (CharShift - FxShift) << ", %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsFixNum(int stackIdx, std::string isFixNumArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, isFixNumArg)
      << "    and $" << FxMask << ", %al\n"
      << "    cmp $" << FxTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsFxZero(int stackIdx, std::string isFxZeroArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, isFxZeroArg)
      << "    cmpl $0, %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsNull(int stackIdx, std::string isNullArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, isNullArg)
      << "    cmpl $" << Null << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsBoolean(int stackIdx, std::string isBooleanArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, isBooleanArg)
      << "    and $" << BoolMask << ", %al\n"
      << "    cmp $" << BoolTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsChar(int stackIdx, std::string isCharArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, isCharArg)
      << "    and $" << CharMask << ", %al\n"
      << "    cmp $" << CharTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitNot(int stackIdx, std::string notArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, notArg)
      << "    cmpl $" << BoolF << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxLogNot(int stackIdx, std::string fxLogNotArg) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, fxLogNotArg)
      << "    xor $" << FxMaskNeg << ", %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxAdd(int stackIdx, std::string lhs, std::string rhs) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, rhs)
      << "    addl " << stackIdx << "(%rsp), %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxSub(int stackIdx, std::string lhs, std::string rhs) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, rhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, lhs)
      << "    subl " << stackIdx << "(%rsp), %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxMul(int stackIdx, std::string lhs, std::string rhs) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, lhs)
      << "    sarl $" << FxShift << ", %eax\n"
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, rhs)
      << "    imul " << stackIdx << "(%rsp), %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxLogOr(int stackIdx, std::string lhs, std::string rhs) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, rhs)
      << "    or " << stackIdx << "(%rsp), %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitFxLogAnd(int stackIdx, std::string lhs, std::string rhs) {
  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, rhs)
      << "    and " << stackIdx << "(%rsp), %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIfExpr(int stackIdx, std::string cond, std::string conseq,
                       std::string alt) {
  std::string altLabel = UniqueLabel();
  std::string endLabel = UniqueLabel();

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, cond)
      << "    cmp $" << BoolF << ", %al\n"
      << "    je " << altLabel << "\n"
      << EmitExpr(stackIdx, conseq)
      << "    jmp " << endLabel << "\n"
      << altLabel << ":\n"
      << EmitExpr(stackIdx, alt)
      << endLabel << ":\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitLogicalExpr(int stackIdx, const std::vector<std::string> &args,
                            bool isAnd) {
  std::ostringstream exprEmissionStream;

  if (args.size() == 0) {
    exprEmissionStream << EmitExpr(stackIdx, "#t");
  } else if (args.size() == 1) {
    exprEmissionStream << EmitExpr(stackIdx, args[0]);
  } else {
    std::ostringstream newExpr;
    newExpr << "(" << (isAnd ? "and" : "or");

    for (int i = 1; i < args.size(); ++i) {
      newExpr << " " << args[i];
    }

    newExpr << ")";

    if (isAnd) {
      exprEmissionStream << EmitIfExpr(stackIdx, args[0], newExpr.str(), "#f");
    } else {
      exprEmissionStream << EmitIfExpr(stackIdx, args[0], "#t", newExpr.str());
    }
  }

  return exprEmissionStream.str();
}

std::string EmitAndExpr(int stackIdx, const std::vector<std::string> &andArgs) {
  return EmitLogicalExpr(stackIdx, andArgs, true);
}

std::string EmitOrExpr(int stackIdx, const std::vector<std::string> &orArgs) {
  return EmitLogicalExpr(stackIdx, orArgs, false);
}

std::string EmitExpr(int stackIdx, std::string expr) {
  assert(IsExpr(expr));

  if (IsImmediate(expr)) {
    std::ostringstream exprEmissionStream;
    // clang-format off
    exprEmissionStream 
        << "    movl $" << ImmediateRep(expr) << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
  }

  std::string primitiveName;
  std::string arg;

  if (TryParseUnaryPrimitive(expr, &primitiveName, &arg)) {
    static std::unordered_map<std::string, TUnaryPrimitiveEmitter>
        unaryEmitters{{"fxadd1", EmitFxAdd1},
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
    return unaryEmitters[primitiveName](stackIdx, arg);
  }

  std::vector<std::string> args;

  if (TryParseBinaryPrimitive(expr, &primitiveName, &args)) {
    assert(args.size() == 2);
    static std::unordered_map<std::string, TBinaryPrimitiveEmitter>
        binaryEmitters{{"fx+", EmitFxAdd},
                       {"fx-", EmitFxSub},
                       {"fx*", EmitFxMul},
                       {"fxlogor", EmitFxLogOr},
                       {"fxlogand", EmitFxLogAnd}};
    assert(binaryEmitters[primitiveName] != nullptr);
    return binaryEmitters[primitiveName](stackIdx, args[0], args[1]);
  }

  std::vector<std::string> ifParts;

  if (TryParseIfExpr(expr, &ifParts)) {
    assert(ifParts.size() == 3);
    return EmitIfExpr(stackIdx, ifParts[0], ifParts[1], ifParts[2]);
  }

  std::vector<std::string> andArgs;

  if (TryParseAndExpr(expr, &andArgs)) {
    return EmitAndExpr(stackIdx, andArgs);
  }

  std::vector<std::string> orArgs;

  if (TryParseOrExpr(expr, &orArgs)) {
    return EmitOrExpr(stackIdx, orArgs);
  }

  assert(false);
}

std::string EmitProgram(std::string programSource) {
  std::ostringstream programEmissionStream;
  // clang-format off
  programEmissionStream 
      << "    .text\n"
      << "    .globl scheme_entry\n"
      << "    .type scheme_entry, @function\n"
      << "scheme_entry:\n"
      << "    movq %rsp, %rcx\n"
      << "    movq 8(%rsp), %rsp\n"
      << EmitExpr(-4, programSource)
      << "    movq %rcx, %rsp\n"
      << "    ret\n";
  // clang-format on

  return programEmissionStream.str();
}

int main(int argc, char *argv[]) {
  std::vector<std::string> testFilePaths{
      "/home/ergawy/repos/inc-compiler/src/tests-1.1-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.2-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.3-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.4-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.5-req.scm"};

  int testCaseCounter = 1;
  int failedTestCaseCounter = 0;

  for (const auto &tfp : testFilePaths) {
    std::ifstream testFile(tfp);

    if (!testFile.is_open()) {
      std::cerr << "Cannot open test file.\n";
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
      std::ostringstream programSourceOutputStream;

      std::string nextProgramSubString;
      testFile >> nextProgramSubString;
      programSourceOutputStream << nextProgramSubString;

      while (true) {
        testFile >> nextProgramSubString;

        if (nextProgramSubString == "=>") {
          break;
        }

        programSourceOutputStream << " " << nextProgramSubString;
      }

      std::string programSource = programSourceOutputStream.str();

      // Parse expected program output.
      std::ostringstream expectedResultOutputStream;

      while (true) {
        std::string nextResultSubString;
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

      std::string expectedResult = expectedResultOutputStream.str();
      expectedResult = expectedResult.substr(1, expectedResult.size() - 4);

      auto programAsm = EmitProgram(programSource);

      std::string testId = "test-" + std::to_string(testCaseCounter);
      std::ofstream programAsmOutputStream(testId + ".s");

      if (!programAsmOutputStream.is_open()) {
        std::cerr << "Couldn't dumpt ASM to output file.";
        return 1;
      }

      programAsmOutputStream << programAsm;
      programAsmOutputStream.close();

      Exec(("gcc /home/ergawy/repos/sil-compiler/runtime.c " + testId +
            ".s -o " + testId + ".out")
               .c_str());
      auto actualResult = Exec(("./" + testId + ".out").c_str());

      std::cout << "[TEST " << testCaseCounter << "]\n";
      std::cout << programSource << "\n";

      std::cout << "\t Expected: " << expectedResult << "\n"
                << "\t Actual  : " << actualResult << "\n";

      if (actualResult == expectedResult) {
        std::cout << "\033[1;32mOK\033[0m\n\n";
      } else {
        std::cout << "\033[1;31mFAILED\033[0m\n\n";
        ++failedTestCaseCounter;
      }

      ++testCaseCounter;
    }
  }

  if (failedTestCaseCounter > 0) {
    std::cout << "\n\033[1;31mFailed/Total: " << failedTestCaseCounter << "/"
              << (testCaseCounter - 1) << "\033[0m\n";
  }

  return 0;
}
