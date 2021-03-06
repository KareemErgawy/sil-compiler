#ifndef PARSE_H
#define PARSE_H

#include "defs.h"

#include <string>
#include <vector>

bool IsFixNum(std::string token);
bool IsBool(std::string token);
bool IsNull(std::string token);
bool IsChar(std::string token);
bool IsImmediate(std::string token);
bool IsVarName(std::string token);
bool TryParseUnaryPrimitive(std::string expr,
                            std::string *outPrimitiveName = nullptr,
                            std::vector<std::string> *outArgs = nullptr);
bool TryParseBinaryPrimitive(std::string expr,
                             std::string *outPrimitiveName = nullptr,
                             std::vector<std::string> *outArgs = nullptr);
bool TryParseTernaryPrimitive(std::string expr,
                              std::string *outPrimitiveName = nullptr,
                              std::vector<std::string> *outArgs = nullptr);
bool TryParseVariableArityPrimitive(
    std::string expr, std::string *outPrimitiveName = nullptr,
    std::vector<std::string> *outArgs = nullptr);
bool TryParseLetExpr(std::string expr, TBindings *outBindings = nullptr,
                     std::vector<std::string> *outLetBody = nullptr);
bool TryParseLetAsteriskExpr(std::string expr,
                             TOrderedBindings *outBindings = nullptr,
                             std::vector<std::string> *outLetBody = nullptr);
bool TryParseLambda(std::string expr,
                    std::vector<std::string> *outVars = nullptr,
                    std::string *outBody = nullptr,
                    std::vector<std::string> *outPossibleFreeVars = nullptr);
bool TryParseProcCallExpr(std::string expr, std::string *outProcName = nullptr,
                          std::vector<std::string> *outParams = nullptr);
bool TryParseLetrec(std::string expr, TBindings *outBindings = nullptr,
                    std::vector<std::string> *outLetBody = nullptr);
bool IsExpr(std::string expr);

#endif
