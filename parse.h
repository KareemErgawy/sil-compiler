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
bool TryParseLetExpr(std::string expr, TBindings *outBindings = nullptr,
                     std::string *outLetBody = nullptr);
bool TryParseLetAsteriskExpr(std::string expr,
                             TOrderedBindings *outBindings = nullptr,
                             std::string *outLetBody = nullptr);
bool TryParseLambdaExpr(std::string expr,
                        std::vector<std::string> *outVars = nullptr,
                        std::string *outBody = nullptr);
bool TryParseProcCallExpr(std::string expr, std::string *outProcName = nullptr,
                          std::vector<std::string> *outParams = nullptr);
bool TryParseLetrecExpr(std::string expr, TBindings *outBindings = nullptr,
                        std::string *outLetBody = nullptr);
bool IsExpr(std::string expr);

#endif
