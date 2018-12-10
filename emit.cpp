#include "emit.h"
#include "parse.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

string EmitExpr(int stackIdx, TEnvironment env, string expr,
                bool isTail = false);

string UniqueLabel(string prefix = "") {
    static unsigned int count = 0;
    return prefix + "_L_" + to_string(count++);
}

string EmitStackSave(int stackIdx) {
    return "    movq %rax, " + to_string(stackIdx) + "(%rsp)\n";
}

string EmitStackLoad(int stackIdx) {
    return "    movq " + to_string(stackIdx) + "(%rsp), %rax\n";
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

string EmitVarRef(TEnvironment env, string expr, bool isTail) {
    auto stackIdx = env.at(expr);
    return "    movq " + to_string(stackIdx) + "(%rsp), %rax\n" +
           (isTail ? "    ret\n" : "");
}

string EmitFxAddImmediate(int stackIdx, TEnvironment env, string fxAddArg,
                          string fxAddImmediate, bool isTail) {
    assert(IsImmediate(fxAddImmediate));

    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, fxAddArg)

                       << "    addq $" << ImmediateRep(fxAddImmediate)
                       << ", %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxAdd1(int stackIdx, TEnvironment env, string fxAdd1Arg,
                  bool isTail) {
    return EmitFxAddImmediate(stackIdx, env, fxAdd1Arg, "1", isTail);
}

string EmitFxSub1(int stackIdx, TEnvironment env, string fxSub1Arg,
                  bool isTail) {
    return EmitFxAddImmediate(stackIdx, env, fxSub1Arg, "-1", isTail);
}

string EmitFixNumToChar(int stackIdx, TEnvironment env, string fixNumToCharArg,
                        bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, fixNumToCharArg)
                       << "    shlq $" << (CharShift - FxShift) << ", %rax\n"
                       << "    orq $" << CharTag << ", %rax\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitCharToFixNum(int stackIdx, TEnvironment env, string charToFixNumArg,
                        bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, charToFixNumArg)
                       << "    shrq $" << (CharShift - FxShift) << ", %rax\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsFixNum(int stackIdx, TEnvironment env, string isFixNumArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, isFixNumArg)

                       << "    and $" << FxMask << ", %al\n"

                       << "    cmp $" << FxTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsFxZero(int stackIdx, TEnvironment env, string isFxZeroArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, isFxZeroArg)
                       << "    cmpq $0, %rax\n"
                       << "    sete %al\n"
                       << "    movzbq %al, %rax\n"
                       << "    sal $" << BoolBit << ", %al\n"
                       << "    or $" << BoolF << ", %al\n"
                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsNull(int stackIdx, TEnvironment env, string isNullArg,
                  bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, isNullArg)

                       << "    cmpq $" << Null << ", %rax\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsBoolean(int stackIdx, TEnvironment env, string isBooleanArg,
                     bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, isBooleanArg)

                       << "    and $" << BoolMask << ", %al\n"

                       << "    cmp $" << BoolTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsChar(int stackIdx, TEnvironment env, string isCharArg,
                  bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, isCharArg)

                       << "    and $" << CharMask << ", %al\n"

                       << "    cmp $" << CharTag << ", %al\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitNot(int stackIdx, TEnvironment env, string notArg, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, notArg)

                       << "    cmpq $" << BoolF << ", %rax\n"

                       << "    sete %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogNot(int stackIdx, TEnvironment env, string fxLogNotArg,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, fxLogNotArg)

                       << "    xor $" << FxMaskNeg << ", %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxAdd(int stackIdx, TEnvironment env, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, rhs)

                       << "    addq " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxSub(int stackIdx, TEnvironment env, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, rhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, lhs)

                       << "    subq " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxMul(int stackIdx, TEnvironment env, string lhs, string rhs,
                 bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, lhs)

                       << "    sarq $" << FxShift << ", %rax\n"

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, rhs)

                       << "    imul " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogOr(int stackIdx, TEnvironment env, string lhs, string rhs,
                   bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, rhs)

                       << "    or " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitFxLogAnd(int stackIdx, TEnvironment env, string lhs, string rhs,
                    bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, rhs)

                       << "    and " << stackIdx << "(%rsp), %rax\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitCmp(int stackIdx, TEnvironment env, string lhs, string rhs,
               string setcc, bool isTail) {
    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, lhs)

                       << "    movq %rax, " << stackIdx << "(%rsp)\n"

                       << EmitExpr(stackIdx - WordSize, env, rhs)

                       << "    cmpq  %rax, " << stackIdx << "(%rsp)\n"

                       << "    " << setcc << " %al\n"

                       << "    movzbq %al, %rax\n"

                       << "    sal $" << BoolBit << ", %al\n"

                       << "    or $" << BoolF << ", %al\n"

                       << (isTail ? "    ret\n" : "");

    return exprEmissionStream.str();
}

string EmitIsEq(int stackIdx, TEnvironment env, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "sete", isTail);
}

string EmitIsCharEq(int stackIdx, TEnvironment env, string lhs, string rhs,
                    bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "sete", isTail);
}

string EmitFxLT(int stackIdx, TEnvironment env, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "setl", isTail);
}

string EmitFxLE(int stackIdx, TEnvironment env, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "setle", isTail);
}

string EmitFxGT(int stackIdx, TEnvironment env, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "setg", isTail);
}

string EmitFxGE(int stackIdx, TEnvironment env, string lhs, string rhs,
                bool isTail) {
    return EmitCmp(stackIdx, env, lhs, rhs, "setge", isTail);
}

string EmitCons(int stackIdx, TEnvironment env, string first, string second,
                bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, first)

           << EmitStackSave(stackIdx)

           << EmitExpr(stackIdx - WordSize, env, second)

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

string EmitIsPair(int stackIdx, TEnvironment env, string isPairArg,
                  bool isTail) {
    ostringstream exprOS;

    // TODO Reduce code duplication in this other xxx? primitives.
    exprOS << EmitExpr(stackIdx, env, isPairArg)

           << "    and $" << PairMask << ", %al\n"

           << "    cmp $" << PairTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitCar(int stackIdx, TEnvironment env, string carArg, bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, carArg, isTail)
           << "    movq -1(%rax), %rax\n"
           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitCdr(int stackIdx, TEnvironment env, string carArg, bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, carArg, isTail)

           << "    movq 7(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitSetPairElement(int stackIdx, TEnvironment env, string oldPair,
                          string newCar, bool isTail, int relOffset) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, newCar)

           << EmitStackSave(stackIdx)

           << EmitExpr(stackIdx - WordSize, env, oldPair)

           << "    movq %rax, %r8\n"

           << EmitStackLoad(stackIdx)

           << "    movq %rax, " << relOffset << "(%r8)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitSetCar(int stackIdx, TEnvironment env, string oldPair, string newCar,
                  bool isTail) {
    return EmitSetPairElement(stackIdx, env, oldPair, newCar, isTail, -1);
}

string EmitSetCdr(int stackIdx, TEnvironment env, string oldPair, string newCdr,
                  bool isTail) {
    return EmitSetPairElement(stackIdx, env, oldPair, newCdr, isTail, 7);
}

string EmitMakeVector(int stackIdx, TEnvironment env, string lengthExpr,
                      bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, lengthExpr)

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

string EmitIsVector(int stackIdx, TEnvironment env, string isVectorArg,
                    bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, isVectorArg)

           << "    and $" << VectorMask << ", %al\n"

           << "    cmp $" << VectorTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorLength(int stackIdx, TEnvironment env, string expr,
                        bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, expr)

           << "    movq -" << VectorTag << "(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorSet(int stackIdx, TEnvironment env, string vec, string idx,
                     string val, bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, val)

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r9\n"

           << EmitExpr(stackIdx, env, vec)

           << "    subq $" << VectorTag << ", %rax\n"

           << "    addq %r9, %rax\n"

           << "    movq %r8, (%rax)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitVectorRef(int stackIdx, TEnvironment env, string vec, string idx,
                     bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, vec)

           << "    subq $" << VectorTag << ", %rax\n"

           << "    addq %r8, %rax\n"

           << "    movq (%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

// TODO Remove duplication between string and vector primitives.
string EmitMakeString(int stackIdx, TEnvironment env, string lengthExpr,
                      bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, lengthExpr)

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

string EmitIsString(int stackIdx, TEnvironment env, string isVectorArg,
                    bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, isVectorArg)

           << "    and $" << StringMask << ", %al\n"

           << "    cmp $" << StringTag << ", %al\n"

           << "    sete %al\n"

           << "    movzbq %al, %rax\n"

           << "    sal $" << BoolBit << ", %al\n"

           << "    or $" << BoolF << ", %al\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringLength(int stackIdx, TEnvironment env, string expr,
                        bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, expr)

           << "    movq -" << StringTag << "(%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringSet(int stackIdx, TEnvironment env, string str, string idx,
                     string val, bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, val)

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r9\n"

           << EmitExpr(stackIdx, env, str)

           << "    subq $" << StringTag << ", %rax\n"

           << "    addq %r9, %rax\n"

           << "    movq %r8, (%rax)\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}

string EmitStringRef(int stackIdx, TEnvironment env, string str, string idx,
                     bool isTail) {
    ostringstream exprOS;

    exprOS << EmitExpr(stackIdx, env, idx)

           << "    sarq $" << FxShift << ", %rax\n"

           << "    imul $" << WordSize << ", %rax\n"

           << "    addq $" << WordSize << ", %rax\n"

           << "    movq %rax, %r8\n"

           << EmitExpr(stackIdx, env, str)

           << "    subq $" << StringTag << ", %rax\n"

           << "    addq %r8, %rax\n"

           << "    movq (%rax), %rax\n"

           << (isTail ? "    ret\n" : "");

    return exprOS.str();
}
string EmitIfExpr(int stackIdx, TEnvironment env, string cond, string conseq,
                  string alt, bool isTail) {
    string altLabel = UniqueLabel();
    string endLabel = UniqueLabel();

    ostringstream exprEmissionStream;
    exprEmissionStream << EmitExpr(stackIdx, env, cond)

                       << "    cmp $" << BoolF << ", %al\n"

                       << "    je " << altLabel << "\n"

                       << EmitExpr(stackIdx, env, conseq, isTail)

                       << "    jmp " << endLabel << "\n"

                       << altLabel << ":\n"

                       << EmitExpr(stackIdx, env, alt, isTail)

                       << endLabel << ":\n";

    return exprEmissionStream.str();
}

string EmitLogicalExpr(int stackIdx, TEnvironment env,
                       const vector<string> &args, bool isAnd, bool isTail) {
    ostringstream exprEmissionStream;

    if (args.size() == 0) {
        exprEmissionStream << EmitExpr(stackIdx, env, "#t", isTail);
    } else if (args.size() == 1) {
        exprEmissionStream << EmitExpr(stackIdx, env, args[0], isTail);
    } else {
        ostringstream newExpr;
        newExpr << "(" << (isAnd ? "and" : "or");

        for (int i = 1; i < args.size(); ++i) {
            newExpr << " " << args[i];
        }

        newExpr << ")";

        if (isAnd) {
            exprEmissionStream << EmitIfExpr(stackIdx, env, args[0],
                                             newExpr.str(), "#f", isTail);
        } else {
            exprEmissionStream << EmitIfExpr(stackIdx, env, args[0], "#t",
                                             newExpr.str(), isTail);
        }
    }

    return exprEmissionStream.str();
}

string EmitAndExpr(int stackIdx, TEnvironment env,
                   const vector<string> &andArgs, bool isTail) {
    return EmitLogicalExpr(stackIdx, env, andArgs, true, isTail);
}

string EmitOrExpr(int stackIdx, TEnvironment env, const vector<string> &orArgs,
                  bool isTail) {
    return EmitLogicalExpr(stackIdx, env, orArgs, false, isTail);
}

string EmitLetExpr(int stackIdx, TEnvironment env, const TBindings &bindings,
                   vector<string> letBody, bool isTail) {
    ostringstream exprEmissionStream;
    int si = stackIdx;
    TEnvironment envExtension;

    for (auto b : bindings) {
        exprEmissionStream << EmitExpr(si, env, b.second)

                           << EmitStackSave(si);
        envExtension.insert({b.first, si});
        si -= WordSize;
    }

    for (auto v : envExtension) {
        env[v.first] = v.second;
    }

    for (int i = 0; i < letBody.size(); ++i) {
        exprEmissionStream << EmitExpr(si, env, letBody[i],
                                       isTail && (i == letBody.size() - 1));
    }

    return exprEmissionStream.str();
}

string EmitLetAsteriskExpr(int stackIdx, TEnvironment env,
                           const TOrderedBindings &bindings,
                           vector<string> letBody, bool isTail) {
    ostringstream exprEmissionStream;
    int si = stackIdx;

    for (auto b : bindings) {
        // clang-format off
        exprEmissionStream
            << EmitExpr(si, env, b.second)
            << EmitStackSave(si);
        // clang-format on
        env[b.first] = si;
        si -= WordSize;
    }

    for (int i = 0; i < letBody.size(); ++i) {
        exprEmissionStream << EmitExpr(si, env, letBody[i],
                                       isTail && (i == letBody.size() - 1));
    }

    return exprEmissionStream.str();
}

static TLambdaTable gLambdaTable;

string EmitSaveProcParamsOnStack(int stackIdx, TEnvironment env,
                                 vector<string> params) {
    ostringstream callOS;
    // Leave room to store the return address on the stack.
    auto paramStackIdx = stackIdx - WordSize;

    for (auto p : params) {
        callOS << EmitExpr(paramStackIdx, env, p)
               << EmitStackSave(paramStackIdx);
        paramStackIdx -= WordSize;
    }

    return callOS.str();
}

string EmitProcCall(int stackIdx, TEnvironment env, string procName,
                    vector<string> params) {
    ostringstream callOS;
    callOS << EmitSaveProcParamsOnStack(stackIdx, env, params);

    // 1 - Adjust the base pointer to the current top of the stack.
    //
    // 2 - Call the procedure.
    //
    // 3 - Adjust the pointer to its original place before the call.
    callOS << "    addq $" << (stackIdx + WordSize) << ", %rsp\n"
           << "    call " << gLambdaTable[procName] << "\n"
           << "    subq $" << (stackIdx + WordSize) << ", %rsp\n";

    return callOS.str();
}

string EmitTailProcCall(int stackIdx, TEnvironment env, string procName,
                        vector<string> params) {
    ostringstream callOS;
    callOS << EmitSaveProcParamsOnStack(stackIdx, env, params);
    auto oldParamStackIdx = stackIdx - WordSize;
    auto newParamStackIdx = -WordSize;

    for (auto p : params) {
        callOS << "    movq " << oldParamStackIdx << "(%rsp), %rax\n"
               << "    movq %rax, " << newParamStackIdx << "(%rsp)\n";
        oldParamStackIdx -= WordSize;
        newParamStackIdx -= WordSize;
    }

    callOS << "    jmp " << gLambdaTable[procName] << "\n";

    return callOS.str();
}

void CreateLambdaTable(const TBindings &lambdas) {
    for (auto l : lambdas) {
        gLambdaTable.insert({l.first, UniqueLabel(l.first)});
    }
}

string EmitLambda(string lambdaLabel, string lambda) {
    vector<string> formalArgs;
    string body;

    if (!TryParseLambda(lambda, &formalArgs, &body)) {
        std::cerr << "Error trying to emit lambda.\n";
        exit(1);
    }

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
             << EmitExpr(stackIdx, lambdaEnv, body, /* isTail */ true);

    return lambdaOS.str();
}

string EmitLetrecLambdas(const TBindings &lambdas) {
    CreateLambdaTable(lambdas);
    ostringstream allLambdasOS;

    for (auto l : lambdas) {
        allLambdasOS << EmitLambda(gLambdaTable[l.first], l.second);
    }

    return allLambdasOS.str();
}

string EmitBegin(int stackIdx, TEnvironment env,
                 const vector<string> &beginExprList, bool isTail) {
    assert(beginExprList.size() > 0);
    ostringstream exprOS;

    for (int i = 0; i < beginExprList.size(); ++i) {
        exprOS << EmitExpr(stackIdx, env, beginExprList[i],
                           isTail && i == (beginExprList.size() - 1));
    }

    return exprOS.str();
}

string EmitExpr(int stackIdx, TEnvironment env, string expr, bool isTail) {
    assert(IsExpr(expr));

    if (IsImmediate(expr)) {
        ostringstream exprEmissionStream;
        exprEmissionStream << "    movq $" << ImmediateRep(expr) << ", %rax\n"
                           << (isTail ? "    ret\n" : "");

        return exprEmissionStream.str();
    }

    if (IsVarName(expr)) {
        return EmitVarRef(env, expr, isTail);
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
            {"string-length", EmitStringLength}};
        assert(unaryEmitters[primitiveName] != nullptr);
        return unaryEmitters[primitiveName](stackIdx, env, unaryArgs[0],
                                            isTail);
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
        return binaryEmitters[primitiveName](stackIdx, env, binaryArgs[0],
                                             binaryArgs[1], isTail);
    }

    vector<string> ternaryArgs;

    if (TryParseTernaryPrimitive(expr, &primitiveName, &ternaryArgs)) {
        assert(ternaryArgs.size() == 3);
        static unordered_map<string, TTernaryPrimitiveEmitter> ternaryEmitters{
            {"if", EmitIfExpr},
            {"vector-set!", EmitVectorSet},
            {"string-set!", EmitStringSet}};
        assert(ternaryEmitters[primitiveName] != nullptr);
        return ternaryEmitters[primitiveName](stackIdx, env, ternaryArgs[0],
                                              ternaryArgs[1], ternaryArgs[2],
                                              isTail);
    }

    vector<string> varArgs;

    if (TryParseVariableArityPrimitive(expr, &primitiveName, &varArgs)) {
        static unordered_map<string, TVaribaleArityPrimitiveEmitter>
            varArityEmitters{
                {"and", EmitAndExpr}, {"or", EmitOrExpr}, {"begin", EmitBegin}};
        assert(varArityEmitters[primitiveName] != nullptr);
        return varArityEmitters[primitiveName](stackIdx, env, varArgs, isTail);
    }

    TBindings bindings;
    vector<string> letBody;

    if (TryParseLetExpr(expr, &bindings, &letBody)) {
        return EmitLetExpr(stackIdx, env, bindings, letBody, isTail);
    }

    TOrderedBindings bindings2;
    vector<string> letBody2;

    if (TryParseLetAsteriskExpr(expr, &bindings2, &letBody2)) {
        return EmitLetAsteriskExpr(stackIdx, env, bindings2, letBody2, isTail);
    }

    vector<string> setVecParts;

    string procName;
    vector<string> params;

    if (TryParseProcCallExpr(expr, &procName, &params)) {
        if (isTail) {
            return EmitTailProcCall(stackIdx, env, procName, params);
        } else {
            return EmitProcCall(stackIdx, env, procName, params);
        }
    }

    assert(false);
}

string EmitProgram(string programSource) {
    ostringstream programEmissionStream;
    programEmissionStream << "    .text\n\n";

    TBindings lambdas;
    vector<string> progBody;

    if (TryParseLetrec(programSource, &lambdas, &progBody)) {
        programEmissionStream << EmitLetrecLambdas(lambdas);
    } else {
        progBody.push_back(programSource);
    }

    programEmissionStream
        << "    .globl scheme_entry\n"
        << "    .type scheme_entry, @function\n"
        << "scheme_entry:\n"
        << "    movq %rdi, %rcx\n"  // Load context* into %rcx.
        << "    movq %rbx, 8(%rcx)\n"
        << "    movq %rsi, 32(%rcx)\n"
        << "    movq %rdi, 40(%rcx)\n"
        << "    movq %rbp, 48(%rcx)\n"
        << "    movq %rsp, 56(%rcx)\n"
        << "    movq %rsi, %rsp\n"   // Load stack space pointer into %rsp.
        << "    movq %rdx, %rbp\n";  // Load heap space pointer into %rbp.

    for (const auto &expr : progBody) {
        programEmissionStream << EmitExpr(-WordSize, TEnvironment(), expr);
    }

    programEmissionStream << "    movq 8(%rcx), %rbx\n"
                          << "    movq 32(%rcx), %rsi\n"
                          << "    movq 40(%rcx), %rdi\n"
                          << "    movq 48(%rcx), %rbp\n"
                          << "    movq 56(%rcx), %rsp\n"
                          << "    ret\n";

    return programEmissionStream.str();
}

