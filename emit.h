#ifndef EMIT_H
#define EMIT_H

#include "defs.h"

#include <string>
#include <vector>

char TokenToChar(std::string token);
int ImmediateRep(std::string token);

std::string EmitFxAddImmediate(int stackIdx, TEnvironment env,
                               std::string fxAddArg, std::string fxAddImmediate,
                               bool isTail = false);
std::string EmitFxAdd1(int stackIdx, TEnvironment env, std::string fxAdd1Arg,
                       bool isTail);
std::string EmitFxSub1(int stackIdx, TEnvironment env, std::string fxSub1Arg,
                       bool isTail);
std::string EmitFixNumToChar(int stackIdx, TEnvironment env,
                             std::string fixNumToCharArg, bool isTail);
std::string EmitCharToFixNum(int stackIdx, TEnvironment env,
                             std::string fixNumToCharArg, bool isTail);
std::string EmitIsFixNum(int stackIdx, TEnvironment env,
                         std::string isFixNumArg, bool isTail);
std::string EmitIsFxZero(int stackIdx, TEnvironment env,
                         std::string isFxZeroArg, bool isTail);
std::string EmitIsNull(int stackIdx, TEnvironment env, std::string isNullArg,
                       bool isTail);
std::string EmitIsBoolean(int stackIdx, TEnvironment env,
                          std::string isBooleanArg, bool isTail);
std::string EmitIsChar(int stackIdx, TEnvironment env, std::string isCharArg,
                       bool isTail);
std::string EmitNot(int stackIdx, TEnvironment env, std::string notArg,
                    bool isTail);
std::string EmitFxLogNot(int stackIdx, TEnvironment env,
                         std::string fxLogNotArg, bool isTail);
std::string EmitFxAdd(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs, bool isTail);
std::string EmitFxSub(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs, bool isTail);
std::string EmitFxMul(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs, bool isTail);
std::string EmitFxLogOr(int stackIdx, TEnvironment env, std::string lhs,
                        std::string rhs, bool isTail);
std::string EmitFxLogAnd(int stackIdx, TEnvironment env, std::string lhs,
                         std::string rhs, bool isTail);
std::string EmitFxCmp(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs, std::string setcc, bool isTail);
std::string EmitFxEq(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs, bool isTail);
std::string EmitFxLT(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs, bool isTail);
std::string EmitFxLE(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs, bool isTail);
std::string EmitFxGT(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs, bool isTail);
std::string EmitFxGE(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs, bool isTail);
std::string EmitIfExpr(int stackIdx, TEnvironment env, std::string cond,
                       std::string conseq, std::string alt,
                       bool isTail = false);
std::string EmitAndExpr(int stackIdx, TEnvironment env,
                        const std::vector<std::string> &andArgs, bool isTail);
std::string EmitOrExpr(int stackIdx, TEnvironment env,
                       const std::vector<std::string> &andArgs, bool isTail);

std::string EmitLetExpr(int stackIdx, TEnvironment env,
                        const TBindings &bindings, std::string letBody,
                        bool isTail);
std::string EmitLetAsteriskExpr(int stackIdx, TEnvironment env,
                                const TOrderedBindings &bindings,
                                std::string letBody, bool isTail);
std::string EmitExpr(int stackIdx, TEnvironment env, std::string expr,
                     bool isTail = false);
std::string EmitProgram(std::string programSource);
std::string UniqueLabel(std::string prefix = "");

#endif
