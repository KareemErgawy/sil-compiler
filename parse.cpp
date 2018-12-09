#include "parse.h"
#include "defs.h"

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace std;

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
    if (token.size() == 0) {
        return false;
    }

    if (!isalpha(token[0])) {
        return false;
    }

    for (auto c : token) {
        if (!isalnum(c)) {
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
        "fxadd1",       "fxsub1",  "fixnum->char", "char->fixnum",
        "fixnum?",      "fxzero?", "null?",        "boolean?",
        "char?",        "not",     "fxlognot",     "pair?",
        "car",          "cdr",     "make-vector",  "vector?",
        "vector-length"};

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

bool TryParseSubExpr(string expr, size_t subExprStart,
                     string *outSubExpr = nullptr,
                     size_t *outSubExprEnd = nullptr) {
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

bool SkipSpaceAndCheckIfEndOfExpr(string expr, size_t *idx) {
    assert(*idx < expr.size());

    for (; *idx < (expr.size() - 1) && isspace(expr[*idx]); ++*idx) {
    }

    return (*idx == (expr.size() - 1));
}

bool TryParseVariableNumOfSubExpr(string expr, size_t startIdx,
                                  vector<string> *outSubExprs,
                                  int expectedNumSubExprs = -1) {
    if (outSubExprs != nullptr) {
        outSubExprs->clear();
    }

    size_t idx = startIdx;
    int numSubExprs = 0;

    while (!SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
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

bool TryParseBinaryPrimitive(string expr, string *outPrimitiveName,
                             vector<string> *outArgs) {
    if (expr.size() < 3) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    static vector<string> binaryPrimitiveNames{
        "fx+",  "fx-",      "fx*",      "fxlogor", "fxlogand",
        "fx=",  "fx<",      "fx<=",     "fx>",     "fx>=",
        "cons", "set-car!", "set-cdr!", "eq?",     "vector-ref"};

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

template <typename TBindings>
bool TryParseLetBindings(string expr, size_t *idx, TBindings *outBindings) {
    assert(idx != nullptr && *idx < expr.size());

    if (SkipSpaceAndCheckIfEndOfExpr(expr, idx)) {
        return false;
    }

    // Parse variable definitions.
    if (expr[*idx] != '(') {
        return false;
    }

    while (true) {
        ++*idx;

        if (expr[*idx] == ')') {
            break;
        }

        if (SkipSpaceAndCheckIfEndOfExpr(expr, idx)) {
            return false;
        }

        if (expr[*idx] != '[') {
            return false;
        }

        ++*idx;

        auto varEnd = expr.find(' ', *idx);

        if (varEnd == string::npos) {
            return false;
        }

        auto varName = expr.substr(*idx, varEnd - *idx);
        *idx = varEnd;

        if (SkipSpaceAndCheckIfEndOfExpr(expr, idx)) {
            return false;
        }

        auto defStart = *idx;

        // Bracketed expression.
        int numOpenBrac = 1;
        ++*idx;

        for (; *idx < expr.size() && numOpenBrac > 0; ++*idx) {
            if (expr[*idx] == '[') {
                ++numOpenBrac;
            }

            if (expr[*idx] == ']') {
                --numOpenBrac;
            }
        }

        if (*idx == expr.size()) {
            return false;
        }

        auto varDef = expr.substr(defStart, *idx - defStart - 1);

        if (!IsExpr(varDef)) {
            return false;
        }

        if (outBindings != nullptr) {
            outBindings->insert(outBindings->end(), {varName, varDef});
        }

        --*idx;
    }

    ++*idx;

    return true;
}

bool TryParseLetBody(string expr, size_t idx, vector<string> *outLetBody) {
    if (SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    return TryParseVariableNumOfSubExpr(expr, idx, outLetBody);
}

bool TryParseLetExpr(string expr, TBindings *outBindings,
                     vector<string> *outLetBody) {
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

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool TryParseLetAsteriskExpr(string expr, TOrderedBindings *outBindings,
                             vector<string> *outLetBody) {
    if (expr.size() < 6) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'l' || expr[2] != 'e' || expr[3] != 't' || expr[4] != '*') {
        return false;
    }

    size_t idx = 5;

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool TryParseLambda(string expr, vector<string> *outFormalArgs,
                    string *outBody) {
    if (expr.size() < 8) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'l' || expr[2] != 'a' || expr[3] != 'm' || expr[4] != 'b' ||
        expr[5] != 'd' || expr[6] != 'a') {
        return false;
    }

    size_t idx = 7;

    if (SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    if (expr[idx] != '(') {
        return false;
    }

    ++idx;

    auto argListEnd = expr.find(')');

    if (argListEnd == string::npos) {
        return false;
    }

    istringstream formalArgReader(expr.substr(idx, argListEnd - idx));

    while (!formalArgReader.eof()) {
        string argName;
        formalArgReader >> argName;

        if (argName.size() == 0) {
            break;
        }

        if (!IsVarName(argName)) {
            return false;
        }

        if (outFormalArgs != nullptr) {
            outFormalArgs->push_back(argName);
        }
    }

    idx = ++argListEnd;

    if (SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    auto body = expr.substr(idx, expr.size() - 1 - idx);

    if (outBody != nullptr) {
        *outBody = body;
    }

    return IsExpr(body);
}

bool TryParseProcCallExpr(string expr, string *outProcName,
                          vector<string> *outParams) {
    if (expr.size() < 3) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    size_t idx = 1;
    auto separator = (expr.find(' ') == string::npos) ? ')' : ' ';

    auto procName = expr.substr(idx, expr.find(separator) - idx);

    if (!IsVarName(procName)) {
        return false;
    }

    if (outProcName != nullptr) {
        *outProcName = procName;
    }

    idx = expr.find(separator);

    if (SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        if (outParams != nullptr) {
            outParams->clear();
        }

        return true;
    }

    return TryParseVariableNumOfSubExpr(expr, idx, outParams);
}

bool TryParseLetrec(string expr, TBindings *outBindings,
                    vector<string> *outLetBody) {
    if (expr.size() < 8) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'l' || expr[2] != 'e' || expr[3] != 't' || expr[4] != 'r' ||
        expr[5] != 'e' || expr[6] != 'c') {
        return false;
    }

    size_t idx = 7;

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool TryParseBegin(string expr, vector<string> *outExprList) {
    if (expr.size() < 7) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr[1] != 'b' || expr[2] != 'e' || expr[3] != 'g' || expr[4] != 'i' ||
        expr[5] != 'n') {
        return false;
    }

    size_t idx = 6;

    return TryParseVariableNumOfSubExpr(expr, idx, outExprList);
}

bool TryParseVectorSet(string expr, vector<string> *outSetVecParts) {
    string syntaxElementName = "vector-set!";
    if (expr.size() < (syntaxElementName.size() + 2)) {
        return false;
    }

    if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
        return false;
    }

    if (expr.substr(1, syntaxElementName.size()) != syntaxElementName) {
        return false;
    }

    size_t idx = syntaxElementName.size() + 1;

    return TryParseVariableNumOfSubExpr(expr, idx, outSetVecParts, 3);
}

bool IsExpr(string expr) {
    return IsImmediate(expr) || IsVarName(expr) ||
           TryParseUnaryPrimitive(expr) || TryParseBinaryPrimitive(expr) ||
           TryParseIfExpr(expr) || TryParseAndExpr(expr) ||
           TryParseOrExpr(expr) || TryParseLetExpr(expr) ||
           TryParseLetAsteriskExpr(expr) || TryParseLambda(expr) ||
           TryParseBegin(expr) || TryParseVectorSet(expr) ||
           TryParseProcCallExpr(expr);
}

