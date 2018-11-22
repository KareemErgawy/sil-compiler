#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

std::string Exec(const char *cmd);

bool IsFixNum(std::string token);
bool IsBool(std::string token);
bool IsNull(std::string token);
bool IsChar(std::string token);
bool IsImmediate(std::string token);

bool IsExpr(std::string expr);
bool IsImmediate(std::string token);
bool TryParseUnaryPrimitive(std::string expr,
                            std::string *outPrimitiveName = nullptr,
                            std::string *outArg = nullptr);
bool IsExpr(std::string expr);

char TokenToChar(std::string token);
int ImmediateRep(std::string token);

using TUnaryPrimitiveEmitter = std::string (*)(std::string);
std::string EmitExpr(std::string expr);
std::string EmitFxAddImmediate(std::string fxAddArg,
                               std::string fxAddImmediate);
std::string EmitFxAdd1(std::string fxAdd1Arg);
std::string EmitFxSub1(std::string fxSub1Arg);
std::string EmitFixNumToChar(std::string fixNumToCharArg);
std::string EmitCharToFixNum(std::string fixNumToCharArg);
std::string EmitIsFixNum(std::string isFixNumArg);
std::string EmitIsFxZero(std::string isFxZeroArg);
std::string EmitIsNull(std::string isNullArg);
std::string EmitIsBoolean(std::string isNullArg);
std::string EmitIsChar(std::string isNullArg);
std::string EmitProgram(std::string programSource);

std::string Exec(const char *cmd) {
  std::array<char, 128> buffer;
  std::string result;
  std::shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
  if (!pipe)
    throw std::runtime_error("popen() failed!");
  while (!feof(pipe.get())) {
    if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
      result += buffer.data();
  }
  return result.substr(0, result.size() - 1);
}

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;
const unsigned int BoolBit = 6;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
const unsigned int CharMask = 0xFF;
const unsigned int CharTag = 0x0F;

const unsigned int WordSize = 4;

const unsigned int FixNumBits = WordSize * 8 - FxShift;
const int FxLower = -std::pow(2, FixNumBits - 1);
const int FxUpper = std::pow(2, FixNumBits - 1) - 1;

bool IsFixNum(std::string token) {
  try {
    auto intVal = std::stoi(token);
    return (FxLower <= intVal) && (intVal <= FxUpper);
  } catch (std::exception &) {
    return false;
  }
}

bool IsBool(std::string token) { return token == "#f" || token == "#t"; }

bool IsNull(std::string token) { return token == "()"; }

bool IsChar(std::string token) {
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

  const std::string specialChars = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
  return std::isalnum(token[2]) ||
         specialChars.find(token[2]) != std::string::npos;
}

bool IsImmediate(std::string token) {
  return IsBool(token) || IsNull(token) || IsChar(token) || IsFixNum(token);
}

bool TryParseUnaryPrimitive(std::string expr, std::string *outPrimitiveName,
                            std::string *outArg) {
  if (expr.size() < 3) {
    return false;
  }

  if (expr[0] != '(' || expr[expr.size() - 1] != ')') {
    return false;
  }

  static std::vector<std::string> unaryPrimitiveNames{
      "fxadd1",  "fxsub1",  "fixnum->char", "char->fixnum",
      "fixnum?", "fxzero?", "null?", "boolean?", "char?"};

  std::string primitiveName = "";
  size_t idx;

  for (idx = 1; idx < (expr.size() - 1) && !std::isspace(expr[idx]); ++idx) {
    primitiveName = primitiveName + expr[idx];
  }

  if (std::find(unaryPrimitiveNames.begin(), unaryPrimitiveNames.end(),
                primitiveName) == unaryPrimitiveNames.end()) {
    return false;
  }

  if (outPrimitiveName != nullptr) {
    *outPrimitiveName = primitiveName;
  }

  // No argument provided.
  if (!std::isspace(expr[idx])) {
    return false;
  }

  for (; idx < (expr.size() - 1) && std::isspace(expr[idx]); ++idx) {
  }

  std::string arg = expr.substr(idx, expr.size() - 1 - idx);

  if (outArg != nullptr) {
    *outArg = arg;
  }

  return IsExpr(arg);
}

bool IsExpr(std::string expr) {
  return IsImmediate(expr) || TryParseUnaryPrimitive(expr);
}

char TokenToChar(std::string token) {
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

int ImmediateRep(std::string token) {
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
  return (std::stoi(token) << FxShift) | FxTag;
}

std::string EmitFxAddImmediate(std::string fxAddArg,
                               std::string fxAddImmediate) {
  assert(IsImmediate(fxAddImmediate));
  std::string argAsm = EmitExpr(fxAddArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream 
      << argAsm 
      << "    addl $" << ImmediateRep(fxAddImmediate) << ", %eax\n";
  // clang-format on 

  return exprEmissionStream.str();

}

std::string EmitFxAdd1(std::string fxAdd1Arg) {
    return EmitFxAddImmediate(fxAdd1Arg, "1");
}

std::string EmitFxSub1(std::string fxSub1Arg) {
    return EmitFxAddImmediate(fxSub1Arg, "-1");
}

std::string EmitFixNumToChar(std::string fixNumToCharArg) {
  std::string argAsm = EmitExpr(fixNumToCharArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    shll $" << (CharShift - FxShift) << ", %eax\n"
      << "    orl $" << CharTag << ", %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitCharToFixNum(std::string charToFixNumArg) {
  std::string argAsm = EmitExpr(charToFixNumArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    shrl $" << (CharShift - FxShift) << ", %eax\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsFixNum(std::string isFixNumArg) {
  std::string argAsm = EmitExpr(isFixNumArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    and $" << FxMask << ", %al\n"
      << "    cmp $" << FxTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsFxZero(std::string isFxZeroArg) {
  std::string argAsm = EmitExpr(isFxZeroArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    cmpl $0, %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsNull(std::string isNullArg) {
  std::string argAsm = EmitExpr(isNullArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    cmpl $" << Null << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsBoolean(std::string isBooleanArg) {
  std::string argAsm = EmitExpr(isBooleanArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    cmpl $" << BoolF << ", %eax\n"
      << "    sete %bl\n"
      << "    movzbl %bl, %ebx\n"
      << "    cmpl $" << BoolT << ", %eax\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    or %bl, %al\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitIsChar(std::string isCharArg) {
  std::string argAsm = EmitExpr(isCharArg);

  std::ostringstream exprEmissionStream;
  // clang-format off
  exprEmissionStream
      << argAsm
      << "    and $" << CharMask << ", %al\n"
      << "    cmp $" << CharTag << ", %al\n"
      << "    sete %al\n"
      << "    movzbl %al, %eax\n"
      << "    sal $" << BoolBit << ", %al\n"
      << "    or $" << BoolF << ", %al\n";
  // clang-format on

  return exprEmissionStream.str();
}

std::string EmitExpr(std::string expr) {
  assert(IsExpr(expr));

  if (IsImmediate(expr)) {
    std::ostringstream exprEmissionStream;
    // clang-format off
    exprEmissionStream 
        << "    movl $" << ImmediateRep(expr) << ", %eax\n";
    // clang-format on

    return exprEmissionStream.str();
  }

  std::string primitiveName;
  std::string arg;

  if (TryParseUnaryPrimitive(expr, &primitiveName, &arg)) {
    static std::unordered_map<std::string, TUnaryPrimitiveEmitter>
        unaryEmitters{{"fxadd1", EmitFxAdd1},
                      {"fxsub1", EmitFxSub1},
                      {"fixnum->char", EmitFixNumToChar},
                      {"char->fixnum", EmitCharToFixNum},
                      {"fixnum?", EmitIsFixNum},
                      {"fxzero?", EmitIsFxZero},
                      {"null?", EmitIsNull},
                      {"boolean?", EmitIsBoolean},
                      {"char?", EmitIsChar}};
    return unaryEmitters[primitiveName](arg);
  }

  assert(false);
}

std::string EmitProgram(std::string programSource) {
  std::ostringstream programEmissionStream;
  // clang-format off
  programEmissionStream 
      << "    .text\n"
      << "    .globl scheme_entry\n"
      << "    .type scheme_entry, @function\n"
      << "scheme_entry:\n"
      << EmitExpr(programSource)
      << "    ret\n";
  // clang-format on

  return programEmissionStream.str();
}

int main(int argc, char *argv[]) {
  std::vector<std::string> testFilePaths{
      "/home/ergawy/repos/inc-compiler/src/tests-1.1-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.2-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.3-req.scm"};

  int testCaseCounter = 1;
  int failedTestCaseCounter = 0;

  for (const auto &tfp : testFilePaths) {
    std::ifstream testFile(tfp);

    if (!testFile.is_open()) {
      std::cerr << "Cannot open test file.\n";
      return 1;
    }

    const size_t MAX_LINE_SIZE = 100;
    char ignoredLine[MAX_LINE_SIZE];

    while (true) {
      char nextChar;
      testFile >> nextChar;

      if (testFile.eof()) {
        break;
      }

      // Test suit header
      if (nextChar == '(') {
        testFile.getline(ignoredLine, MAX_LINE_SIZE);
        continue;
      }

      if (nextChar == ')') {
        continue;
      }

      // Comment.
      if (nextChar == ';') {
        testFile.getline(ignoredLine, MAX_LINE_SIZE);
        continue;
      }

      //
      // Parse test case.
      //

      // Parse program source.
      std::ostringstream programSourceOutputStream;

      std::string nextProgramSubString;
      testFile >> nextProgramSubString;
      programSourceOutputStream << nextProgramSubString;

      while (true) {
        testFile >> nextProgramSubString;

        if (nextProgramSubString == "=>") {
          break;
        }

        programSourceOutputStream << " " << nextProgramSubString;
      }

      std::string programSource = programSourceOutputStream.str();

      // Parse expected program output.
      std::ostringstream expectedResultOutputStream;

      while (true) {
        std::string nextResultSubString;
        testFile >> nextResultSubString;
        bool lastSubString = nextResultSubString.back() == ']';
        expectedResultOutputStream
            << (lastSubString ? nextResultSubString.substr(
                                    0, nextResultSubString.size() - 1)
                              : nextResultSubString);

        if (lastSubString) {
          break;
        }
      }

      std::string expectedResult = expectedResultOutputStream.str();
      expectedResult = expectedResult.substr(1, expectedResult.size() - 4);

      auto programAsm = EmitProgram(programSource);

      std::string testId = "test-" + std::to_string(testCaseCounter);
      std::ofstream programAsmOutputStream(testId + ".s");

      if (!programAsmOutputStream.is_open()) {
        std::cerr << "Couldn't dumpt ASM to output file.";
        return 1;
      }

      programAsmOutputStream << programAsm;
      programAsmOutputStream.close();

      Exec(("clang /home/ergawy/repos/sil-compiler/runtime.c " + testId +
            ".s -o " + testId + ".out")
               .c_str());
      auto actualResult = Exec(("./" + testId + ".out").c_str());

      std::cout << "[TEST " << testCaseCounter << "]\n";
      std::cout << programSource << "\n";

      std::cout << "\t Expected: " << expectedResult << "\n"
                << "\t Actual  : " << actualResult << "\n";

      if (actualResult == expectedResult) {
        std::cout << "\033[1;32mOK\033[0m\n\n";
      } else {
        std::cout << "\033[1;31mFAILED\033[0m\n\n";
        ++failedTestCaseCounter;
      }

      ++testCaseCounter;
    }
  }

  if (failedTestCaseCounter > 0) {
    std::cout << "\n\033[1;31mFailed/Total: " << failedTestCaseCounter << "/"
              << testCaseCounter << "\033[0m\n";
  }

  return 0;
}
