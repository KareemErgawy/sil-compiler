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

bool IsProperlyParenthesized(string expr) {
    return expr[0] == '(' && expr[expr.size() - 1] == ')';
}

bool TryParseSubExpr(string expr, int subExprStart,
                     string *outSubExpr = nullptr,
                     int *outSubExprEnd = nullptr) {
    // The (- 1) accounts for the closing ) of expr.
    assert(subExprStart < expr.size() - 1);

    int idx = subExprStart;
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

bool SkipSpaceAndCheckIfEndOfExpr(string expr, int *idx) {
    assert(*idx < expr.size());

    for (; *idx < (expr.size() - 1) && isspace(expr[*idx]); ++*idx) {
    }

    return (*idx == (expr.size() - 1));
}

bool TryParseVariableNumOfSubExpr(string expr, int startIdx,
                                  vector<string> *outSubExprs,
                                  int expectedNumSubExprs = -1) {
    if (outSubExprs != nullptr) {
        outSubExprs->clear();
    }

    int idx = startIdx;
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

bool TryParsePrimitve(int arity, const vector<string> &primList, string expr,
                      string *outPrimitiveName, vector<string> *outArgs) {
    if (expr.size() < 3) {
        return false;
    }

    if (!IsProperlyParenthesized(expr)) {
        return false;
    }

    string primitiveName = "";
    int idx;

    for (idx = 1; idx < (expr.size() - 1) && !isspace(expr[idx]); ++idx) {
        primitiveName = primitiveName + expr[idx];
    }

    if (find(primList.begin(), primList.end(), primitiveName) ==
        primList.end()) {
        return false;
    }

    if (outPrimitiveName != nullptr) {
        *outPrimitiveName = primitiveName;
    }

    return TryParseVariableNumOfSubExpr(expr, idx, outArgs, arity);
}

bool TryParseUnaryPrimitive(string expr, string *outPrimitiveName,
                            vector<string> *outArgs) {
    static const vector<string> unaryPrimitiveNames{
        "fxadd1",        "fxsub1",      "fixnum->char", "char->fixnum",
        "fixnum?",       "fxzero?",     "null?",        "boolean?",
        "char?",         "not",         "fxlognot",     "pair?",
        "car",           "cdr",         "make-vector",  "vector?",
        "vector-length", "make-string", "string?",      "string-length",
        "procedure?"};

    return TryParsePrimitve(1, unaryPrimitiveNames, expr, outPrimitiveName,
                            outArgs);
}

bool TryParseBinaryPrimitive(string expr, string *outPrimitiveName,
                             vector<string> *outArgs) {
    static const vector<string> binaryPrimitiveNames{
        "fx+",      "fx-",  "fx*",        "fxlogor",    "fxlogand", "fx=",
        "fx<",      "fx<=", "fx>",        "fx>=",       "cons",     "set-car!",
        "set-cdr!", "eq?",  "vector-ref", "string-ref", "char=",    "set!"};

    return TryParsePrimitve(2, binaryPrimitiveNames, expr, outPrimitiveName,
                            outArgs);
}

bool TryParseTernaryPrimitive(string expr, string *outPrimitiveName,
                              vector<string> *outArgs) {
    static const vector<string> ternaryPrimitiveNames{"if", "vector-set!",
                                                      "string-set!"};
    return TryParsePrimitve(3, ternaryPrimitiveNames, expr, outPrimitiveName,
                            outArgs);
}

bool TryParseVariableArityPrimitive(string expr, string *outPrimitiveName,
                                    vector<string> *outArgs) {
    static const vector<string> primitiveNames{"and", "or", "begin"};
    return TryParsePrimitve(-1, primitiveNames, expr, outPrimitiveName,
                            outArgs);
}

int TryParseSyntaxElementPrefix(string syntaxElementName, string expr) {
    if (expr.size() < (syntaxElementName.size() + 2)) {
        return -1;
    }

    if (!IsProperlyParenthesized(expr)) {
        return -1;
    }

    if (expr.substr(1, syntaxElementName.size()) != syntaxElementName) {
        return -1;
    }

    return syntaxElementName.size() + 1;
}

template <typename TBindings>
bool TryParseLetBindings(string expr, int *idx, TBindings *outBindings) {
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

bool TryParseLetBody(string expr, int idx, vector<string> *outLetBody) {
    if (SkipSpaceAndCheckIfEndOfExpr(expr, &idx)) {
        return false;
    }

    return TryParseVariableNumOfSubExpr(expr, idx, outLetBody);
}

bool TryParseLetExpr(string expr, TBindings *outBindings,
                     vector<string> *outLetBody) {
    auto idx = TryParseSyntaxElementPrefix("let", expr);

    if (idx == -1) {
        return false;
    }

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool TryParseLetAsteriskExpr(string expr, TOrderedBindings *outBindings,
                             vector<string> *outLetBody) {
    auto idx = TryParseSyntaxElementPrefix("let*", expr);

    if (idx == -1) {
        return false;
    }

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

void CollectLambdaFreeVars(string body, const vector<string> &formalArgs,
                           vector<string> *outPossibleFreeVars) {
    assert(IsExpr(body));
    static const string delimitiers = "()[] ";

    for (int i = 0; i < body.size();) {
        if (delimitiers.find(body[i]) != string::npos) {
            ++i;
            continue;
        }

        int tokenStart = i;

        for (; (delimitiers.find(body[i]) == string::npos) && (i < body.size());
             ++i) {
        }

        assert(tokenStart < i);

        string token = body.substr(tokenStart, i - tokenStart);

        if (token == "lambda" || token == "let") {
            int numOfParen = 1;

            for (; i < body.size() && numOfParen > 0; ++i) {
                if (body[i] == '(') {
                    ++numOfParen;
                    continue;
                }

                if (body[i] == ')') {
                    --numOfParen;
                    continue;
                }
            }

            continue;
        }

        // NOTE I don't check whether the token matches one of the language
        // primitives in order to leave room for adding the capability to redefe
        // these primitives in the future.
        if (IsVarName(token)) {
            auto isFormalArg = std::find(formalArgs.begin(), formalArgs.end(),
                                         token) != formalArgs.end();
            auto isAlreadyAdded =
                std::find(outPossibleFreeVars->begin(),
                          outPossibleFreeVars->end(),
                          token) != outPossibleFreeVars->end();

            if (!isFormalArg && !isAlreadyAdded) {
                outPossibleFreeVars->push_back(token);
            }
        }
    }
}

bool TryParseLambda(string expr, vector<string> *outFormalArgs, string *outBody,
                    vector<string> *outPossibleFreeVars) {
    auto idx = TryParseSyntaxElementPrefix("lambda", expr);

    if (idx == -1) {
        return false;
    }

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

    vector<string> lambdaBodyExprs;
    if (!TryParseVariableNumOfSubExpr(expr, idx, &lambdaBodyExprs)) {
        return false;
    }

    auto body = string("(begin ") + expr.substr(idx, expr.size() - idx);

    if (outBody != nullptr) {
        *outBody = body;
    }

    if (outFormalArgs != nullptr && outPossibleFreeVars != nullptr) {
        CollectLambdaFreeVars(body, *outFormalArgs, outPossibleFreeVars);
    }

    return true;
}

bool TryParseProcCallExpr(string expr, string *outProcName,
                          vector<string> *outParams) {
    if (expr.size() < 3) {
        return false;
    }

    if (!IsProperlyParenthesized(expr)) {
        return false;
    }

    int idx = 1;

    string procName;
    TryParseSubExpr(expr, idx, &procName, &idx);

    // if (!IsVarName(procName)) {
    //    return false;
    //}

    if (outProcName != nullptr) {
        *outProcName = procName;
    }

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
    auto idx = TryParseSyntaxElementPrefix("letrec", expr);

    if (idx == -1) {
        return false;
    }

    return TryParseLetBindings(expr, &idx, outBindings) &&
           TryParseLetBody(expr, idx, outLetBody);
}

bool IsExpr(string expr) {
    return IsImmediate(expr) || IsVarName(expr) ||
           TryParseUnaryPrimitive(expr) || TryParseBinaryPrimitive(expr) ||
           TryParseTernaryPrimitive(expr) ||
           TryParseVariableArityPrimitive(expr) || TryParseLetExpr(expr) ||
           TryParseLetAsteriskExpr(expr) || TryParseLambda(expr) ||
           TryParseProcCallExpr(expr);
}

