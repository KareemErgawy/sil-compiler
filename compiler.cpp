#include "defs.h"
#include "emit.h"

#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

// TODO Reduce code repetition on parsing functions: TryParseUnaryPrimitive,
// TryParseBinaryPrimitive, TryParseIfExpr, TryParseAndExpr, TryParseOrExpr.

// TODO Switch emitted code to intel syntax.

// TODO Parsing is too sensitive to whitespace. Make it more robust.

using namespace std;

string Exec(const char *cmd) {
    array<char, 128> buffer;
    string result;
    shared_ptr<FILE> pipe(popen(cmd, "r"), pclose);
    if (!pipe) throw runtime_error("popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
            result += buffer.data();
    }
    return result.substr(0, result.size() - 1);
}

int main(int argc, char *argv[]) {
    vector<string> testFilePaths{
        "/home/ergawy/repos/inc-compiler/src/tests-1.1-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.2-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.3-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.4-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.5-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.6-req.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.6-opt.scm",
        "/home/ergawy/repos/inc-compiler/src/tests-1.7-req.scm"};

    int testCaseCounter = 1;
    int failedTestCaseCounter = 0;

    for (const auto &tfp : testFilePaths) {
        ifstream testFile(tfp);

        if (!testFile.is_open()) {
            cerr << "Cannot open test file.\n";
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
            ostringstream programSourceOutputStream;

            string nextProgramSubString;
            testFile >> nextProgramSubString;
            programSourceOutputStream << nextProgramSubString;

            while (true) {
                testFile >> nextProgramSubString;

                if (nextProgramSubString == "=>") {
                    break;
                }

                programSourceOutputStream << " " << nextProgramSubString;
            }

            string programSource = programSourceOutputStream.str();

            // Parse expected program output.
            ostringstream expectedResultOutputStream;

            while (true) {
                string nextResultSubString;
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

            string expectedResult = expectedResultOutputStream.str();
            expectedResult =
                expectedResult.substr(1, expectedResult.size() - 4);

            auto programAsm = EmitProgram(programSource);

            string testId = "test-" + to_string(testCaseCounter);
            ofstream programAsmOutputStream(testId + ".s");

            if (!programAsmOutputStream.is_open()) {
                cerr << "Couldn't dumpt ASM to output file.";
                return 1;
            }

            programAsmOutputStream << programAsm;
            programAsmOutputStream.close();

            Exec(("gcc /home/ergawy/repos/sil-compiler/runtime.c " + testId +
                  ".s -o " + testId + ".out")
                     .c_str());
            auto actualResult = Exec(("./" + testId + ".out").c_str());

            cout << "[TEST " << testCaseCounter << "]\n";
            cout << programSource << "\n";

            cout << "\t Expected: " << expectedResult << "\n"
                 << "\t Actual  : " << actualResult << "\n";

            if (actualResult == expectedResult) {
                cout << "\033[1;32mOK\033[0m\n\n";
            } else {
                cout << "\033[1;31mFAILED\033[0m\n\n";
                ++failedTestCaseCounter;
            }

            ++testCaseCounter;
        }
    }

    if (failedTestCaseCounter > 0) {
        cout << "\n\033[1;31mFailed/Total: " << failedTestCaseCounter << "/"
             << (testCaseCounter - 1) << "\033[0m\n";
    }

    return 0;
}
