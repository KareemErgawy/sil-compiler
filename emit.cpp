#include "emit.h"
#include "parse.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

ostringstream gAllLambdasOS;

string EmitExpr(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string expr,
                bool isTail = false);

string UniqueLabel(string prefix = "") {
    static unsigned int count = 0;
    return prefix + "_L_" + to_string(count++);
}

string EmitStackSave(int stackIdx, string sourceReg = "rax") {
    return "    # Stack save.\n    movq %" + sourceReg + ", " +
           to_string(stackIdx) + "(%rsp)\n";
}

string EmitStackLoad(int stackIdx, string targetReg = "rax") {
    return "    # Stack load.\n    movq " + to_string(stackIdx) + "(%rsp), %" +
           targetReg + "\n";
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
    assert(IsFixNum(token));
    return (stoi(token) << FxShift) | FxTag;
}

bool IsLocalOrCapturedVar(TEnvironment env, const TClosureEnvironment& closEnv,
                          string possibleVarName) {
    return env.count(possibleVarName) || closEnv.count(possibleVarName);
}

string EmitVarRef(TEnvironment env, const TClosureEnvironment& closEnv,
                  string expr, bool isTail) {
    assert(IsLocalOrCapturedVar(env, closEnv, expr));

    if (env.count(expr)) {
        auto stackIdx = env.at(expr);
        return "    # Local var ref: " + expr + ".\n" + "    movq " +
               to_string(stackIdx) + "(%rsp), %rax\n" +
               (isTail ? "    ret\n" : "");
    }

    auto heapIdx = closEnv.at(expr) - ClosureTag;
    return "    # Free var ref: " + expr + ".\n" + "    movq " +
           to_string(heapIdx) + "(%rdi), %rax\n" + (isTail ? "    ret\n" : "");
}

string EmitFxAddImmediate(int stackIdx, TEnvironment env,
                          const TClosureEnvironment& closEnv, string fxAddArg,
                          string fxAddImmediate, bool isTail) {
    assert(IsImmediate(fxAddImmediate));

    ostringstream exprEmissionStream;
    exprEmissionStream << "    # Add imm.\n"

                       << EmitExpr(stackIdx, env, closEnv, fxAddArg)

                       << "    addq $" << ImmediateRep(fxAddImmediate)
                       << ", %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxAdd1(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string fxAdd1Arg,
                  bool isTail) {
    return EmitFxAddImmediate(stackIdx, env, closEnv, fxAdd1Arg, "1", isTail);
}

string EmitFxSub1(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string fxSub1Arg,
                  bool isTail) {
    return EmitFxAddImmediate(stackIdx, env, closEnv, fxSub1Arg, "-1", isTail);
}

string EmitFixNumToChar(int stackIdx, TEnvironment env,
                        const TClosureEnvironment& closEnv,
                        string fixNumToCharArg, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fixnum->char.\n"
                       << EmitExpr(stackIdx, env, closEnv, fixNumToCharArg)
                       << "    shlq $" << (CharShift - FxShift) << ", %rax\n"
                       << "    orq $" << CharTag << ", %rax\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitCharToFixNum(int stackIdx, TEnvironment env,
                        const TClosureEnvironment& closEnv,
                        string charToFixNumArg, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # char->fixnum.\n"
                       << EmitExpr(stackIdx, env, closEnv, charToFixNumArg)
                       << "    shrq $" << (CharShift - FxShift) << ", %rax\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsFixNum(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string isFixNumArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fixnum?.\n"

                       << EmitExpr(stackIdx, env, closEnv, isFixNumArg)

                       << "    and $" << FxMask << ", %al\n"

                       << "    cmp $" << FxTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsFxZero(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string isFxZeroArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # zero?.\n"
                       << EmitExpr(stackIdx, env, closEnv, isFxZeroArg)
                       << "    cmpq $0, %rax\n"
                       << "    sete %al\n"
                       << "    movzbq %al, %rax\n"
                       << "    sal $" << BoolBit << ", %al\n"
                       << "    or $" << BoolF << ", %al\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsNull(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string isNullArg,
                  bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # null?.\n"

                       << EmitExpr(stackIdx, env, closEnv, isNullArg)

                       << "    cmpq $" << Null << ", %rax\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsBoolean(int stackIdx, TEnvironment env,
                     const TClosureEnvironment& closEnv, string isBooleanArg,
                     bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # boolean?.\n"

                       << EmitExpr(stackIdx, env, closEnv, isBooleanArg)

                       << "    and $" << BoolMask << ", %al\n"

                       << "    cmp $" << BoolTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsChar(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string isCharArg,
                  bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # char?.\n"

                       << EmitExpr(stackIdx, env, closEnv, isCharArg)

                       << "    and $" << CharMask << ", %al\n"

                       << "    cmp $" << CharTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitNot(int stackIdx, TEnvironment env,
               const TClosureEnvironment& closEnv, string notArg, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # not.\n"

                       << EmitExpr(stackIdx, env, closEnv, notArg)

                       << "    cmpq $" << BoolF << ", %rax\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogNot(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string fxLogNotArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fxlognot.\n"

                       << EmitExpr(stackIdx, env, closEnv, fxLogNotArg)

                       << "    xor $" << FxMaskNeg << ", %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxAdd(int stackIdx, TEnvironment env,
                 const TClosureEnvironment& closEnv, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fx+.\n"

                       << EmitExpr(stackIdx, env, closEnv, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, rhs)

                       << "    addq " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxSub(int stackIdx, TEnvironment env,
                 const TClosureEnvironment& closEnv, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fx-.\n"

                       << EmitExpr(stackIdx, env, closEnv, rhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, lhs)

                       << "    subq " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxMul(int stackIdx, TEnvironment env,
                 const TClosureEnvironment& closEnv, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fx*.\n"

                       << EmitExpr(stackIdx, env, closEnv, lhs)

                       << "    sarq $" << FxShift << ", %rax\n"

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, rhs)

                       << "    imul " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogOr(int stackIdx, TEnvironment env,
                   const TClosureEnvironment& closEnv, string lhs, string rhs,
                   bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fxlogor.\n"

                       << EmitExpr(stackIdx, env, closEnv, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, rhs)

                       << "    or " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogAnd(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string lhs, string rhs,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # fxlogand.\n"

                       << EmitExpr(stackIdx, env, closEnv, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, rhs)

                       << "    and " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitCmp(int stackIdx, TEnvironment env,
               const TClosureEnvironment& closEnv, string lhs, string rhs,
               string setcc, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << "    # cmp(" << setcc << ").\n"

                       << EmitExpr(stackIdx, env, closEnv, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, closEnv, rhs)

                       << "    cmpq  %rax, " << stackIdx << "(%rsp)\n"

                       << "    " << setcc << " %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsEq(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "sete", isTail);
}

string EmitIsCharEq(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string lhs, string rhs,
                    bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "sete", isTail);
}

string EmitFxLT(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "setl", isTail);
}

string EmitFxLE(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "setle", isTail);
}

string EmitFxGT(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "setg", isTail);
}

string EmitFxGE(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, closEnv, lhs, rhs, "setge", isTail);
}

string EmitCons(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string first, string second,
                bool isTail) {
    ostringstream exprOS;

    exprOS << "    # cons.\n"

           << EmitExpr(stackIdx, env, closEnv, first)

           << EmitStackSave(stackIdx)

           << EmitExpr(stackIdx - WordSize, env, closEnv, second)

           << "    movq %rax, 8(%rbp)\n"  // Store cdr a word after car.

           << EmitStackLoad(stackIdx)

           << "    movq %rax, (%rbp)\n"  // Store car at the next avaiable heap
                                         // pointer.

           << "    movq %rbp, %rax\n"  // Store the pair pointer into %rax.

           << "    orq $" << PairTag << ", %rax\n"

           << "    addq $16, %rbp\n"  // Move the heap forward by pair size.

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitIsPair(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string isPairArg,
                  bool isTail) {
    ostringstream exprOS;

    // TODO Reduce code duplication in this other xxx? primitives.
    exprOS << "    # pair?.\n"

           << EmitExpr(stackIdx, env, closEnv, isPairArg)

           << "    and $" << HeapObjMask << ", %al\n"

           << "    cmp $" << PairTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitCar(int stackIdx, TEnvironment env,
               const TClosureEnvironment& closEnv, string carArg, bool isTail) {
    ostringstream exprOS;

    exprOS << "    # car.\n"
           << EmitExpr(stackIdx, env, closEnv, carArg, isTail)
           << "    movq -1(%rax), %rax\n"
           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitCdr(int stackIdx, TEnvironment env,
               const TClosureEnvironment& closEnv, string carArg, bool isTail) {
    ostringstream exprOS;

    exprOS << "    # cdr.\n"

           << EmitExpr(stackIdx, env, closEnv, carArg, isTail)

           << "    movq 7(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitSetPairElement(int stackIdx, TEnvironment env,
                          const TClosureEnvironment& closEnv, string oldPair,
                          string newCar, bool isTail, int relOffset) {
    ostringstream exprOS;

    exprOS << ((relOffset == -1) ? "    # set-car!.\n" : "    # set-cdr!.\n")

           << EmitExpr(stackIdx, env, closEnv, newCar)

           << EmitStackSave(stackIdx)

           << EmitExpr(stackIdx - WordSize, env, closEnv, oldPair)

           << "    movq %rax, %r8\n"

           << EmitStackLoad(stackIdx)

           << "    movq %rax, " << relOffset << "(%r8)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitSetCar(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string oldPair,
                  string newCar, bool isTail) {
    return EmitSetPairElement(stackIdx, env, closEnv, oldPair, newCar, isTail,
                              -1);
}

string EmitSetCdr(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string oldPair,
                  string newCdr, bool isTail) {
    return EmitSetPairElement(stackIdx, env, closEnv, oldPair, newCdr, isTail,
                              7);
}

string EmitMakeVector(int stackIdx, TEnvironment env,
                      const TClosureEnvironment& closEnv, string lengthExpr,
                      bool isTail) {
    ostringstream exprOS;

    exprOS << "    # make-vector.\n"

           << EmitExpr(stackIdx, env, closEnv, lengthExpr)

           << "    movq %rax, (%rbp)\n"

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rbp, %r8\n"

           << "    addq %rax, %rbp\n"

           << "    movq %r8, %rax\n"

           << "    orq $" << VectorTag << ", %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitIsVector(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string isVectorArg,
                    bool isTail) {
    ostringstream exprOS;

    exprOS << "    # vector?.\n"

           << EmitExpr(stackIdx, env, closEnv, isVectorArg)

           << "    and $" << HeapObjMask << ", %al\n"

           << "    cmp $" << VectorTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorLength(int stackIdx, TEnvironment env,
                        const TClosureEnvironment& closEnv, string expr,
                        bool isTail) {
    ostringstream exprOS;

    exprOS << "    # vector-length.\n"

           << EmitExpr(stackIdx, env, closEnv, expr)

           << "    movq -" << VectorTag << "(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorSet(int stackIdx, TEnvironment env,
                     const TClosureEnvironment& closEnv, string vec, string idx,
                     string val, bool isTail) {
    ostringstream exprOS;

    exprOS << "    # vector-set!.\n"

           << EmitExpr(stackIdx, env, closEnv, val)

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, closEnv, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r9\n"

           << EmitExpr(stackIdx, env, closEnv, vec)

           << "    subq $" << VectorTag << ", %rax\n"

           << "    addq %r9, %rax\n"

           << "    movq %r8, (%rax)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorRef(int stackIdx, TEnvironment env,
                     const TClosureEnvironment& closEnv, string vec, string idx,
                     bool isTail) {
    ostringstream exprOS;

    exprOS << "    # vector-ref.\n"

           << EmitExpr(stackIdx, env, closEnv, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, closEnv, vec)

           << "    subq $" << VectorTag << ", %rax\n"

           << "    addq %r8, %rax\n"

           << "    movq (%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

// TODO Remove duplication between string and vector primitives.
string EmitMakeString(int stackIdx, TEnvironment env,
                      const TClosureEnvironment& closEnv, string lengthExpr,
                      bool isTail) {
    ostringstream exprOS;

    exprOS << "    # make-string.\n"

           << EmitExpr(stackIdx, env, closEnv, lengthExpr)

           << "    movq %rax, (%rbp)\n"

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rbp, %r8\n"

           << "    addq %rax, %rbp\n"

           << "    movq %r8, %rax\n"

           << "    orq $" << StringTag << ", %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitIsString(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string isVectorArg,
                    bool isTail) {
    ostringstream exprOS;

    exprOS << "    # string?.\n"

           << EmitExpr(stackIdx, env, closEnv, isVectorArg)

           << "    and $" << HeapObjMask << ", %al\n"

           << "    cmp $" << StringTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringLength(int stackIdx, TEnvironment env,
                        const TClosureEnvironment& closEnv, string expr,
                        bool isTail) {
    ostringstream exprOS;

    exprOS << "    # string-length.\n"

           << EmitExpr(stackIdx, env, closEnv, expr)

           << "    movq -" << StringTag << "(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitIsProcedure(int stackIdx, TEnvironment env,
                       const TClosureEnvironment& closEnv, string isProcArg,
                       bool isTail) {
    ostringstream exprOS;

    exprOS << "    # procedure?.\n"

           << EmitExpr(stackIdx, env, closEnv, isProcArg)

           << "    and $" << HeapObjMask << ", %al\n"

           << "    cmp $" << ClosureTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringSet(int stackIdx, TEnvironment env,
                     const TClosureEnvironment& closEnv, string str, string idx,
                     string val, bool isTail) {
    ostringstream exprOS;

    exprOS << "    # string-set!.\n"

           << EmitExpr(stackIdx, env, closEnv, val)

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, closEnv, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r9\n"

           << EmitExpr(stackIdx, env, closEnv, str)

           << "    subq $" << StringTag << ", %rax\n"

           << "    addq %r9, %rax\n"

           << "    movq %r8, (%rax)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringRef(int stackIdx, TEnvironment env,
                     const TClosureEnvironment& closEnv, string str, string idx,
                     bool isTail) {
    ostringstream exprOS;

    exprOS << "    # string-ref.\n"

           << EmitExpr(stackIdx, env, closEnv, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, closEnv, str)

           << "    subq $" << StringTag << ", %rax\n"

           << "    addq %r8, %rax\n"

           << "    movq (%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}
string EmitIfExpr(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv, string cond,
                  string conseq, string alt, bool isTail) {
    string altLabel = UniqueLabel();
    string endLabel = UniqueLabel();

    ostringstream exprEmissionStream;
    exprEmissionStream << "    # if.\n"

                       << EmitExpr(stackIdx, env, closEnv, cond)

                       << "    cmp $" << BoolF << ", %al\n"

                       << "    je " << altLabel << "\n"

                       << EmitExpr(stackIdx, env, closEnv, conseq, isTail);

    if (!isTail) {
        exprEmissionStream << "    jmp " << endLabel << "\n";
    }

    exprEmissionStream << altLabel << ":\n"

                       << EmitExpr(stackIdx, env, closEnv, alt, isTail);

    if (!isTail) {
        exprEmissionStream << endLabel << ":\n";
    }

    return exprEmissionStream.str();
}

string EmitLogicalExpr(int stackIdx, TEnvironment env,
                       const TClosureEnvironment& closEnv,
                       const vector<string>& args, bool isAnd, bool isTail) {
    ostringstream exprEmissionStream;

    if (args.size() == 0) {
        exprEmissionStream << EmitExpr(stackIdx, env, closEnv, "#t", isTail);
    } else if (args.size() == 1) {
        exprEmissionStream << EmitExpr(stackIdx, env, closEnv, args[0], isTail);
    } else {
        ostringstream newExpr;
        newExpr << "(" << (isAnd ? "and" : "or");

        for (int i = 1; i < args.size(); ++i) {
            newExpr << " " << args[i];
        }

        newExpr << ")";

        if (isAnd) {
            exprEmissionStream << "    # and.\n"

                               << EmitIfExpr(stackIdx, env, closEnv, args[0],
                                             newExpr.str(), "#f", isTail);
        } else {
            exprEmissionStream << "    # or.\n"

                               << EmitIfExpr(stackIdx, env, closEnv, args[0],
                                             "#t", newExpr.str(), isTail);
        }
    }

    return exprEmissionStream.str();
}

string EmitAndExpr(int stackIdx, TEnvironment env,
                   const TClosureEnvironment& closEnv,
                   const vector<string>& andArgs, bool isTail) {
    return EmitLogicalExpr(stackIdx, env, closEnv, andArgs, true, isTail);
}

string EmitOrExpr(int stackIdx, TEnvironment env,
                  const TClosureEnvironment& closEnv,
                  const vector<string>& orArgs, bool isTail) {
    return EmitLogicalExpr(stackIdx, env, closEnv, orArgs, false, isTail);
}

string EmitLetExpr(int stackIdx, TEnvironment env,
                   const TClosureEnvironment& closEnv,
                   const TBindings& bindings, vector<string> letBody,
                   bool isTail) {
    ostringstream exprEmissionStream;
    int si = stackIdx;
    TEnvironment envExtension;

    exprEmissionStream << "    # let.\n";

    for (auto b : bindings) {
        exprEmissionStream << "      # binding: " << b.first << ".\n"

                           << EmitExpr(si, env, closEnv, b.second)

                           << EmitStackSave(si);
        envExtension.insert({b.first, si});
        si -= WordSize;
    }

    for (auto v : envExtension) {
        env[v.first] = v.second;
    }

    for (int i = 0; i < letBody.size(); ++i) {
        exprEmissionStream << EmitExpr(si, env, closEnv, letBody[i],
                                       isTail && (i == letBody.size() - 1));
    }

    return exprEmissionStream.str();
}

string EmitLetAsteriskExpr(int stackIdx, TEnvironment env,
                           const TClosureEnvironment& closEnv,
                           const TOrderedBindings& bindings,
                           vector<string> letBody, bool isTail) {
    ostringstream exprEmissionStream;
    int si = stackIdx;

    exprEmissionStream << "    # let*.\n";

    for (auto b : bindings) {
        exprEmissionStream << "      # Binding: " << b.first << ".\n"
                           << EmitExpr(si, env, closEnv, b.second)
                           << EmitStackSave(si);
        env[b.first] = si;
        si -= WordSize;
    }

    for (int i = 0; i < letBody.size(); ++i) {
        exprEmissionStream << EmitExpr(si, env, closEnv, letBody[i],
                                       isTail && (i == letBody.size() - 1));
    }

    return exprEmissionStream.str();
}

static TLambdaTable gLambdaTable;

string EmitSaveProcParamsOnStack(int stackIdx, TEnvironment env,
                                 const TClosureEnvironment& closEnv,
                                 vector<string> params) {
    ostringstream callOS;
    // Leave room to store the return address and %rbp on the stack.
    auto paramStackIdx = stackIdx - (WordSize * 2);

    for (auto p : params) {
        callOS << "    # Emit param on stack: " << p << ".\n"
               << EmitExpr(paramStackIdx, env, closEnv, p)
               << EmitStackSave(paramStackIdx);
        paramStackIdx -= WordSize;
    }

    return callOS.str();
}

string EmitProcCall(int stackIdx, TEnvironment env,
                    const TClosureEnvironment& closEnv, string procName,
                    vector<string> params) {
    ostringstream callOS;
    callOS << EmitSaveProcParamsOnStack(stackIdx, env, closEnv, params);

    // 1 - Adjust the base pointer to the current top of the stack.
    //
    // 2 - Call the procedure.
    //
    // 3 - Adjust the pointer to its original place before the call.

    callOS << "    # Call: " + procName << ".\n";

    if (!IsLocalOrCapturedVar(env, closEnv, procName)) {
        callOS << "    addq $" << stackIdx << ", %rsp\n"
               << "    call " << gLambdaTable[procName] << "\n";
    } else {
        callOS << EmitStackSave(stackIdx, "rdi");

        callOS << EmitVarRef(env, closEnv, procName, false)

               << "    movq %rax, %rdi\n"

               << "    movq -" << ClosureTag << "(%rax), %rax\n"

               << "    addq $" << stackIdx << ", %rsp\n"

               << "    call *%rax\n";
    }

    callOS << "    subq $" << stackIdx << ", %rsp\n";

    if (IsLocalOrCapturedVar(env, closEnv, procName)) {
        callOS << EmitStackLoad(stackIdx, "rdi");
        stackIdx += WordSize;
    }

    return callOS.str();
}

string EmitTailProcCall(int stackIdx, TEnvironment env,
                        const TClosureEnvironment& closEnv, string procName,
                        vector<string> params) {
    ostringstream callOS;
    callOS << "    # Tail call: " << procName << ".\n"
           << EmitSaveProcParamsOnStack(stackIdx, env, closEnv, params);
    auto oldParamStackIdx = stackIdx - WordSize * 2;
    auto newParamStackIdx = -WordSize;

    if (IsLocalOrCapturedVar(env, closEnv, procName)) {
        callOS << EmitVarRef(env, closEnv, procName, false)

               << "    movq %rax, %rdi\n"

               << "    movq -" << ClosureTag << "(%rax), %rbx\n";
    }

    for (auto p : params) {
        callOS << "    movq " << oldParamStackIdx << "(%rsp), %rax\n"
               << "    movq %rax, " << newParamStackIdx << "(%rsp)\n";
        oldParamStackIdx -= WordSize;
        newParamStackIdx -= WordSize;
    }

    if (!IsLocalOrCapturedVar(env, closEnv, procName)) {
        callOS << "    jmp " << gLambdaTable[procName] << "\n";
    } else {
        callOS << "    jmp *%rbx\n";
    }

    return callOS.str();
}

void CreateLambdaTable(const TBindings& lambdas) {
    for (auto l : lambdas) {
        gLambdaTable.insert({l.first, UniqueLabel(l.first)});
    }
}

string EmitLambda(string lambdaLabel, const vector<string>& formalArgs,
                  string body, const TClosureEnvironment& closEnv) {
    TEnvironment lambdaEnv;
    auto stackIdx = -WordSize;

    for (auto arg : formalArgs) {
        lambdaEnv[arg] = stackIdx;
        stackIdx -= WordSize;
    }

    ostringstream lambdaOS;
    lambdaOS << "    .globl " << lambdaLabel << "\n"
             << "    .type " << lambdaLabel << ", @function\n"
             << lambdaLabel << ":\n"
             << EmitExpr(stackIdx, lambdaEnv, closEnv, body, /* isTail */ true);

    return lambdaOS.str();
}

string EmitLetrecLambdas(const TBindings& lambdas) {
    CreateLambdaTable(lambdas);
    ostringstream allLambdasOS;

    for (auto l : lambdas) {
        vector<string> formalArgs;
        string body;

        if (!TryParseLambda(l.second, &formalArgs, &body)) {
            std::cerr << "Error trying to emit lambda.\n";
            exit(1);
        }

        allLambdasOS << EmitLambda(gLambdaTable[l.first], formalArgs, body,
                                   TClosureEnvironment());
    }

    return allLambdasOS.str();
}

string EmitBegin(int stackIdx, TEnvironment env,
                 const TClosureEnvironment& closEnv,
                 const vector<string>& beginExprList, bool isTail) {
    assert(beginExprList.size() > 0);
    ostringstream exprOS;

    for (int i = 0; i < beginExprList.size(); ++i) {
        exprOS << EmitExpr(stackIdx, env, closEnv, beginExprList[i],
                           isTail && i == (beginExprList.size() - 1));
    }

    return exprOS.str();
}

string EmitExpr(int stackIdx, TEnvironment env,
                const TClosureEnvironment& closEnv, string expr, bool isTail) {
    assert(IsExpr(expr));

    if (IsImmediate(expr)) {
        ostringstream exprEmissionStream;
        exprEmissionStream << "    movq $" << ImmediateRep(expr) << ", %rax\n"
                           << (isTail ? "    ret\n" : "");

        return exprEmissionStream.str();
    }

    if (IsVarName(expr)) {
        return EmitVarRef(env, closEnv, expr, isTail);
    }

    string primitiveName;
    vector<string> unaryArgs;

    if (TryParseUnaryPrimitive(expr, &primitiveName, &unaryArgs)) {
        assert(unaryArgs.size() == 1);
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
            {"fxlognot", EmitFxLogNot},
            {"pair?", EmitIsPair},
            {"car", EmitCar},
            {"cdr", EmitCdr},
            {"make-vector", EmitMakeVector},
            {"vector?", EmitIsVector},
            {"vector-length", EmitVectorLength},
            {"make-string", EmitMakeString},
            {"string?", EmitIsString},
            {"string-length", EmitStringLength},
            {"procedure?", EmitIsProcedure}};
        assert(unaryEmitters[primitiveName] != nullptr);
        return unaryEmitters[primitiveName](stackIdx, env, closEnv,
                                            unaryArgs[0], isTail);
    }

    vector<string> binaryArgs;

    if (TryParseBinaryPrimitive(expr, &primitiveName, &binaryArgs)) {
        assert(binaryArgs.size() == 2);
        static unordered_map<string, TBinaryPrimitiveEmitter> binaryEmitters{
            {"fx+", EmitFxAdd},
            {"fx-", EmitFxSub},
            {"fx*", EmitFxMul},
            {"fxlogor", EmitFxLogOr},
            {"fxlogand", EmitFxLogAnd},
            {"fx=", EmitIsEq},
            {"fx<", EmitFxLT},
            {"fx<=", EmitFxLE},
            {"fx>", EmitFxGT},
            {"fx>=", EmitFxGE},
            {"cons", EmitCons},
            {"set-car!", EmitSetCar},
            {"set-cdr!", EmitSetCdr},
            {"eq?", EmitIsEq},
            {"vector-ref", EmitVectorRef},
            {"string-ref", EmitStringRef},
            {"char=", EmitIsCharEq}};
        assert(binaryEmitters[primitiveName] != nullptr);
        return binaryEmitters[primitiveName](
            stackIdx, env, closEnv, binaryArgs[0], binaryArgs[1], isTail);
    }

    vector<string> ternaryArgs;

    if (TryParseTernaryPrimitive(expr, &primitiveName, &ternaryArgs)) {
        assert(ternaryArgs.size() == 3);
        static unordered_map<string, TTernaryPrimitiveEmitter> ternaryEmitters{
            {"if", EmitIfExpr},
            {"vector-set!", EmitVectorSet},
            {"string-set!", EmitStringSet}};
        assert(ternaryEmitters[primitiveName] != nullptr);
        return ternaryEmitters[primitiveName](stackIdx, env, closEnv,
                                              ternaryArgs[0], ternaryArgs[1],
                                              ternaryArgs[2], isTail);
    }

    vector<string> varArgs;

    if (TryParseVariableArityPrimitive(expr, &primitiveName, &varArgs)) {
        static unordered_map<string, TVaribaleArityPrimitiveEmitter>
            varArityEmitters{
                {"and", EmitAndExpr}, {"or", EmitOrExpr}, {"begin", EmitBegin}};
        assert(varArityEmitters[primitiveName] != nullptr);
        return varArityEmitters[primitiveName](stackIdx, env, closEnv, varArgs,
                                               isTail);
    }

    TBindings bindings;
    vector<string> letBody;

    if (TryParseLetExpr(expr, &bindings, &letBody)) {
        return EmitLetExpr(stackIdx, env, closEnv, bindings, letBody, isTail);
    }

    TOrderedBindings bindings2;
    vector<string> letBody2;

    if (TryParseLetAsteriskExpr(expr, &bindings2, &letBody2)) {
        return EmitLetAsteriskExpr(stackIdx, env, closEnv, bindings2, letBody2,
                                   isTail);
    }

    vector<string> formalArgs;
    string body;
    vector<string> possibleFreeVars;

    if (TryParseLambda(expr, &formalArgs, &body, &possibleFreeVars)) {
        auto label = UniqueLabel();
        // TODO Get the naming for lambda and closure related parts right.
        ostringstream exprOS;
        exprOS
            << "    # Create lambda object.\n"
            << "    leaq " << label << "(%rip), %rax\n"
            << "    movq %rax, (%rbp)\n";  // Save the lambda ptr on the heap.
        auto numFreeVars = 0;
        TClosureEnvironment newClosEnv;

        for (int i = 0; i < possibleFreeVars.size(); ++i) {
            if (IsLocalOrCapturedVar(env, newClosEnv, possibleFreeVars[i])) {
                auto fvHeapIdx = numFreeVars * WordSize;
                exprOS << "      # Capturing: " << possibleFreeVars[i] << ".\n"
                       << EmitVarRef(env, newClosEnv, possibleFreeVars[i],
                                     false)
                       << "    movq %rax, " << (fvHeapIdx + WordSize)
                       << "(%rbp)\n";
                newClosEnv[possibleFreeVars[i]] = fvHeapIdx + WordSize;
                ++numFreeVars;
            }
        }

        exprOS << "    movq %rbp, %rax\n"

               << "    orq $" << ClosureTag << ", %rax\n"

               << "    addq $" << (WordSize + (numFreeVars * WordSize))
               << ", %rbp\n";

        gAllLambdasOS << EmitLambda(label, formalArgs, body, newClosEnv)
                      << "\n\n";

        return exprOS.str();
    }

    string procName;
    vector<string> params;

    if (TryParseProcCallExpr(expr, &procName, &params)) {
        if (isTail) {
            return EmitTailProcCall(stackIdx, env, closEnv, procName, params);
        } else {
            return EmitProcCall(stackIdx, env, closEnv, procName, params);
        }
    }

    assert(false);
}

string EmitProgram(string programSource) {
    gAllLambdasOS.str("");
    gAllLambdasOS.clear();

    ostringstream programEmissionStream;
    programEmissionStream << "    .text\n\n";

    TBindings lambdas;
    vector<string> progBody;

    if (TryParseLetrec(programSource, &lambdas, &progBody)) {
        programEmissionStream << EmitLetrecLambdas(lambdas);
    } else {
        progBody.push_back(programSource);
    }

    ostringstream schemeEntryOS;

    for (const auto& expr : progBody) {
        schemeEntryOS << EmitExpr(-WordSize, TEnvironment(),
                                  TClosureEnvironment(), expr);
    }

    programEmissionStream
        << gAllLambdasOS.str()

        << "    .globl scheme_entry\n"
        << "    .type scheme_entry, @function\n"
        << "scheme_entry:\n"
        << "    movq %rdi, %rcx\n"  // Load context* into %rcx.
        << "    movq %rbx, 8(%rcx)\n"
        << "    movq %rsi, 32(%rcx)\n"
        << "    movq %rdi, 40(%rcx)\n"
        << "    movq %rbp, 48(%rcx)\n"
        << "    movq %rsp, 56(%rcx)\n"
        << "    movq %rsi, %rsp\n"  // Load stack space pointer into %rsp.
        << "    movq %rdx, %rbp\n"  // Load heap space pointer into %rbp.

        << schemeEntryOS.str()

        << "    movq 8(%rcx), %rbx\n"
        << "    movq 32(%rcx), %rsi\n"
        << "    movq 40(%rcx), %rdi\n"
        << "    movq 48(%rcx), %rbp\n"
        << "    movq 56(%rcx), %rsp\n"
        << "    ret\n";

    return programEmissionStream.str();
}

