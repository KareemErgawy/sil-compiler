#include "emit.h"
#include "parse.h"

#include <cassert>
#include <iostream>
#include <sstream>

using namespace std;

string UniqueLabel(string prefix) {
    static unsigned int count = 0;
    return prefix + "_L_" + to_string(count++);
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
    return (stoi(token) << FxShift) | FxTag;
}

string EmitVarRef(TEnvironment env, string expr) {
    auto stackIdx = env.at(expr);
    return "    movl " + to_string(stackIdx) + "(%rsp), %eax\n";
}

string EmitFxAddImmediate(int stackIdx, TEnvironment env, string fxAddArg,
                          string fxAddImmediate) {
    assert(IsImmediate(fxAddImmediate));

    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream 
      << EmitExpr(stackIdx, env, fxAddArg)
      << "    addl $" << ImmediateRep(fxAddImmediate) << ", %eax\n";
  // clang-format on 

  return exprEmissionStream.str();

}

string EmitFxAdd1(int stackIdx, TEnvironment env, string fxAdd1Arg) {
    return EmitFxAddImmediate(stackIdx, env, fxAdd1Arg, "1");
}

string EmitFxSub1(int stackIdx, TEnvironment env, string fxSub1Arg) {
    return EmitFxAddImmediate(stackIdx, env, fxSub1Arg, "-1");
}

string EmitFixNumToChar(int stackIdx, TEnvironment env, string fixNumToCharArg) {
  ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, fixNumToCharArg)
      << "    shll $" << (CharShift - FxShift) << ", %eax\n"
      << "    orl $" << CharTag << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitCharToFixNum(int stackIdx, TEnvironment env,
                        string charToFixNumArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, charToFixNumArg)
      << "    shrl $" << (CharShift - FxShift) << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsFixNum(int stackIdx, TEnvironment env, string isFixNumArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isFixNumArg)
      << "    and $" << FxMask << ", %al\n"
      << "    cmp $" << FxTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsFxZero(int stackIdx, TEnvironment env, string isFxZeroArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isFxZeroArg)
      << "    cmpl $0, %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsNull(int stackIdx, TEnvironment env, string isNullArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isNullArg)
      << "    cmpl $" << Null << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsBoolean(int stackIdx, TEnvironment env, string isBooleanArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isBooleanArg)
      << "    and $" << BoolMask << ", %al\n"
      << "    cmp $" << BoolTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitIsChar(int stackIdx, TEnvironment env, string isCharArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, isCharArg)
      << "    and $" << CharMask << ", %al\n"
      << "    cmp $" << CharTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitNot(int stackIdx, TEnvironment env, string notArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, notArg)
      << "    cmpl $" << BoolF << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogNot(int stackIdx, TEnvironment env, string fxLogNotArg) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, fxLogNotArg)
      << "    xor $" << FxMaskNeg << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxAdd(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    addl " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxSub(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, rhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, lhs)
      << "    subl " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxMul(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    sarl $" << FxShift << ", %eax\n"
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    imul " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogOr(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    or " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxLogAnd(int stackIdx, TEnvironment env, string lhs, string rhs) {
    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    and " << stackIdx << "(%rsp), %eax\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxCmp(int stackIdx, TEnvironment env, string lhs, string rhs,
                 string setcc) {
    ostringstream exprEmissionStream;
    // clang-format off
    exprEmissionStream
      << EmitExpr(stackIdx, env, lhs)
      << "    movl %eax, " << stackIdx << "(%rsp)\n"
      << EmitExpr(stackIdx - WordSize, env, rhs)
      << "    cmpl  %eax, " << stackIdx << "(%rsp)\n"
      << "    " << setcc << " %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitFxEq(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "sete");
}

string EmitFxLT(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setl");
}

string EmitFxLE(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setle");
}

string EmitFxGT(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setg");
}

string EmitFxGE(int stackIdx, TEnvironment env, string lhs, string rhs) {
    return EmitFxCmp(stackIdx, env, lhs, rhs, "setge");
}

string EmitIfExpr(int stackIdx, TEnvironment env, string cond, string conseq,
                  string alt) {
    string altLabel = UniqueLabel();
    string endLabel = UniqueLabel();

    ostringstream exprEmissionStream;
    // clang-format off
  exprEmissionStream
      << EmitExpr(stackIdx, env, cond)
      << "    cmp $" << BoolF << ", %al\n"
      << "    je " << altLabel << "\n"
      << EmitExpr(stackIdx, env, conseq)
      << "    jmp " << endLabel << "\n"
      << altLabel << ":\n"
      << EmitExpr(stackIdx, env, alt)
      << endLabel << ":\n";
    // clang-format on

    return exprEmissionStream.str();
}

string EmitLogicalExpr(int stackIdx, TEnvironment env,
                       const vector<string> &args, bool isAnd) {
    ostringstream exprEmissionStream;

    if (args.size() == 0) {
        exprEmissionStream << EmitExpr(stackIdx, env, "#t");
    } else if (args.size() == 1) {
        exprEmissionStream << EmitExpr(stackIdx, env, args[0]);
    } else {
        ostringstream newExpr;
        newExpr << "(" << (isAnd ? "and" : "or");

        for (int i = 1; i < args.size(); ++i) {
            newExpr << " " << args[i];
        }

        newExpr << ")";

        if (isAnd) {
            exprEmissionStream
                << EmitIfExpr(stackIdx, env, args[0], newExpr.str(), "#f");
        } else {
            exprEmissionStream
                << EmitIfExpr(stackIdx, env, args[0], "#t", newExpr.str());
        }
    }

    return exprEmissionStream.str();
}

string EmitAndExpr(int stackIdx, TEnvironment env,
                   const vector<string> &andArgs) {
    return EmitLogicalExpr(stackIdx, env, andArgs, true);
}

string EmitOrExpr(int stackIdx, TEnvironment env,
                  const vector<string> &orArgs) {
    return EmitLogicalExpr(stackIdx, env, orArgs, false);
}

string EmitStackSave(int stackIdx) {
    return "    movl %eax, " + to_string(stackIdx) + "(%rsp)\n";
}

string EmitLetExpr(int stackIdx, TEnvironment env, const TBindings &bindings,
                   string letBody) {
    ostringstream exprEmissionStream;
    int si = stackIdx;
    TEnvironment envExtension;

    for (auto b : bindings) {
        // clang-format off
        exprEmissionStream
            << EmitExpr(si, env, b.second)
            << EmitStackSave(si);
        // clang-format on
        envExtension.insert({b.first, si});
        si -= WordSize;
    }

    for (auto v : envExtension) {
        env[v.first] = v.second;
    }

    exprEmissionStream << EmitExpr(si, env, letBody);

    return exprEmissionStream.str();
}

string EmitLetAsteriskExpr(int stackIdx, TEnvironment env,
                           const TOrderedBindings &bindings, string letBody) {
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

    exprEmissionStream << EmitExpr(si, env, letBody);

    return exprEmissionStream.str();
}

TLambdaTable CreateLambdaTable(const TBindings &lambdas) {
    TLambdaTable result;
    for (auto l : lambdas) {
        result.insert({l.first, UniqueLabel()});
    }

    return result;
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
             << EmitExpr(stackIdx, lambdaEnv, body);

    return lambdaOS.str();
}

string EmitLetrecLambdas(const TBindings &lambdas) {
    auto lambdaTable = CreateLambdaTable(lambdas);
    ostringstream allLambdasOS;

    for (auto l : lambdas) {
        allLambdasOS << EmitLambda(lambdaTable[l.first], l.second)
                     << "    ret\n\n";
    }

    return allLambdasOS.str();
}

string EmitExpr(int stackIdx, TEnvironment env, string expr) {
    assert(IsExpr(expr));

    if (IsImmediate(expr)) {
        ostringstream exprEmissionStream;
        // clang-format off
    exprEmissionStream 
        << "    movl $" << ImmediateRep(expr) << ", %eax\n";
        // clang-format on

        return exprEmissionStream.str();
    }

    if (IsVarName(expr)) {
        return EmitVarRef(env, expr);
    }

    string primitiveName;
    string arg;

    if (TryParseUnaryPrimitive(expr, &primitiveName, &arg)) {
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
            {"fxlognot", EmitFxLogNot}};
        return unaryEmitters[primitiveName](stackIdx, env, arg);
    }

    vector<string> args;

    if (TryParseBinaryPrimitive(expr, &primitiveName, &args)) {
        assert(args.size() == 2);
        static unordered_map<string, TBinaryPrimitiveEmitter> binaryEmitters{
            {"fx+", EmitFxAdd},         {"fx-", EmitFxSub},
            {"fx*", EmitFxMul},         {"fxlogor", EmitFxLogOr},
            {"fxlogand", EmitFxLogAnd}, {"fx=", EmitFxEq},
            {"fx<", EmitFxLT},          {"fx<=", EmitFxLE},
            {"fx>", EmitFxGT},          {"fx>=", EmitFxGE}};
        assert(binaryEmitters[primitiveName] != nullptr);
        return binaryEmitters[primitiveName](stackIdx, env, args[0], args[1]);
    }

    vector<string> ifParts;

    if (TryParseIfExpr(expr, &ifParts)) {
        assert(ifParts.size() == 3);
        return EmitIfExpr(stackIdx, env, ifParts[0], ifParts[1], ifParts[2]);
    }

    vector<string> andArgs;

    if (TryParseAndExpr(expr, &andArgs)) {
        return EmitAndExpr(stackIdx, env, andArgs);
    }

    vector<string> orArgs;

    if (TryParseOrExpr(expr, &orArgs)) {
        return EmitOrExpr(stackIdx, env, orArgs);
    }

    TBindings bindings;
    string letBody;

    if (TryParseLetExpr(expr, &bindings, &letBody)) {
        return EmitLetExpr(stackIdx, env, bindings, letBody);
    }

    TOrderedBindings bindings2;
    string letBody2;

    if (TryParseLetAsteriskExpr(expr, &bindings2, &letBody2)) {
        return EmitLetAsteriskExpr(stackIdx, env, bindings2, letBody2);
    }

    if (TryParseProcCallExpr(expr)) {
        // TODO Implement procedure calls.
        return "";
    }

    assert(false);
}

string EmitProgram(string programSource) {
    ostringstream programEmissionStream;
    programEmissionStream << "    .text\n\n";

    TBindings lambdas;
    string progBody;

    if (TryParseLetrec(programSource, &lambdas, &progBody)) {
        programEmissionStream << EmitLetrecLambdas(lambdas);
    } else {
        progBody = programSource;
    }

    programEmissionStream << "    .globl scheme_entry\n"
                          << "    .type scheme_entry, @function\n"
                          << "scheme_entry:\n"
                          << "    movq %rsp, %rcx\n"
                          << "    movq %rdi, %rsp\n"
                          << EmitExpr(-WordSize, TEnvironment(), progBody)
                          << "    movq %rcx, %rsp\n"
                          << "    ret\n";

    return programEmissionStream.str();
}

