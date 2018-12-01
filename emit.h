#ifndef EMIT_H
#define EMIT_H

#include "defs.h"

#include <string>
#include <vector>

char TokenToChar(std::string token);
int ImmediateRep(std::string token);

std::string EmitFxAddImmediate(int stackIdx, TEnvironment env,
                               std::string fxAddArg,
                               std::string fxAddImmediate);
std::string EmitFxAdd1(int stackIdx, TEnvironment env, std::string fxAdd1Arg);
std::string EmitFxSub1(int stackIdx, TEnvironment env, std::string fxSub1Arg);
std::string EmitFixNumToChar(int stackIdx, TEnvironment env,
                             std::string fixNumToCharArg);
std::string EmitCharToFixNum(int stackIdx, TEnvironment env,
                             std::string fixNumToCharArg);
std::string EmitIsFixNum(int stackIdx, TEnvironment env,
                         std::string isFixNumArg);
std::string EmitIsFxZero(int stackIdx, TEnvironment env,
                         std::string isFxZeroArg);
std::string EmitIsNull(int stackIdx, TEnvironment env, std::string isNullArg);
std::string EmitIsBoolean(int stackIdx, TEnvironment env,
                          std::string isBooleanArg);
std::string EmitIsChar(int stackIdx, TEnvironment env, std::string isCharArg);
std::string EmitNot(int stackIdx, TEnvironment env, std::string notArg);
std::string EmitFxLogNot(int stackIdx, TEnvironment env,
                         std::string fxLogNotArg);
std::string EmitFxAdd(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs);
std::string EmitFxSub(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs);
std::string EmitFxMul(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs);
std::string EmitFxLogOr(int stackIdx, TEnvironment env, std::string lhs,
                        std::string rhs);
std::string EmitFxLogAnd(int stackIdx, TEnvironment env, std::string lhs,
                         std::string rhs);
std::string EmitFxCmp(int stackIdx, TEnvironment env, std::string lhs,
                      std::string rhs, std::string setcc);
std::string EmitFxEq(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs);
std::string EmitFxLT(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs);
std::string EmitFxLE(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs);
std::string EmitFxGT(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs);
std::string EmitFxGE(int stackIdx, TEnvironment env, std::string lhs,
                     std::string rhs);
std::string EmitIfExpr(int stackIdx, TEnvironment env, std::string cond,
                       std::string conseq, std::string alt);
std::string EmitAndExpr(int stackIdx, TEnvironment env,
                        const std::vector<std::string> &andArgs);
std::string EmitExpr(int stackIdx, TEnvironment env, std::string expr);
std::string EmitProgram(std::string programSource);
std::string UniqueLabel(std::string prefix = "");

#endif
