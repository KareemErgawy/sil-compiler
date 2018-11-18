#include <algorithm>
#include <array>
#include <cassert>
#include <cmath>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
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

std::string EmitProgram(std::string programSource) {
  std::ostringstream programEmissionStream;
  programEmissionStream << "    .text\n"
                        << "    .globl scheme_entry\n"
                        << "    .type scheme_entry, @function\n"
                        << "scheme_entry:\n"
                        << "    movl $" << ImmediateRep(programSource)
                        << ", %eax\n"
                        << "    ret\n";

  return programEmissionStream.str();
}

int main(int argc, char *argv[]) {
  std::vector<std::string> testFilePaths{
      "/home/ergawy/repos/inc-compiler/src/tests-1.1-req.scm",
      "/home/ergawy/repos/inc-compiler/src/tests-1.2-req.scm"};

  int testCaseCounter = 1;

  for (const auto &tfp : testFilePaths) {
    std::ifstream testFile(tfp);

    if (!testFile.is_open()) {
      std::cerr << "Cannot open test file.\n";
      return 1;
    }

    const size_t MAX_LINE_SIZE = 100;
    char ignoredLine[MAX_LINE_SIZE];
    testFile.getline(ignoredLine, MAX_LINE_SIZE);

    while (true) {
      char nextChar;
      testFile >> nextChar;

      if (nextChar == ')') {
        break;
      }

      if (nextChar == ';') {
        testFile.getline(ignoredLine, MAX_LINE_SIZE);
        continue;
      }

      //
      // Parse test case.
      //

      // Parse program source.
      std::ostringstream programSourceOutputStream;

      while (true) {
        std::string nextProgramSubString;
        testFile >> nextProgramSubString;

        if (nextProgramSubString == "=>") {
          break;
        }

        programSourceOutputStream << nextProgramSubString;
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

      std::cout << "[TEST " << testCaseCounter << "] ";

      if (actualResult == expectedResult) {
        std::cout << "OK.\n";
      } else {
        std::cout << "FAILED\n";
      }

      std::cout << "\t Expected: " << expectedResult << "\n"
                << "\t Actual  : " << actualResult << "\n";

      ++testCaseCounter;
    }
  }
  return 0;
}
