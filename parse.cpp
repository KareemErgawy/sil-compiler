#include "parse.h"
#include "defs.h"

#include <algorithm>
#include <cassert>
#include <iostream>

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

bool TryParseLetBindings(string expr, size_t *idx, TBindings *outBindings) {
    assert(idx != nullptr && *idx < expr.size());

    if (skipSpaceAndCheckIfEndOfExpr(expr, idx)) {
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

        if (skipSpaceAndCheckIfEndOfExpr(expr, idx)) {
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

        if (skipSpaceAndCheckIfEndOfExpr(expr, idx)) {
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
            outBindings->insert({varName, varDef});
        }

        --*idx;
    }

    ++*idx;

    return true;
}

bool TryParseLetBody(string expr, size_t idx, string *outLetBody) {
    if (skipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    auto letBody = expr.substr(idx, expr.size() - 1 - idx);

    if (outLetBody != nullptr) {
        *outLetBody = letBody;
    }

    return IsExpr(letBody);
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

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool TryParseLetAsteriskExpr(string expr, TBindings *outBindings,
                             string *outLetBody) {
    std::cout << expr << "\n";
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

bool IsExpr(string expr) {
    return IsImmediate(expr) || IsVarName(expr) ||
           TryParseUnaryPrimitive(expr) || TryParseBinaryPrimitive(expr) ||
           TryParseIfExpr(expr) || TryParseAndExpr(expr) ||
           TryParseOrExpr(expr) || TryParseLetExpr(expr) ||
           TryParseLetAsteriskExpr(expr);
}

