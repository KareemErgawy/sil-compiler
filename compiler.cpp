#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

class ParseUtils {
public:
  // Constructs the string_view of the token that extends between the beginning
  // of the line represented by aStmt and position of the operator at
  // aStmt[aOpPos]
  static std::string_view ExtractTokenViewBefore(const std::string &aStmt,
                                                 std::size_t aOpPos) {
    auto varNameStart = std::find_if(aStmt.begin(), aStmt.begin() + aOpPos,
                                     [](char c) { return !std::isspace(c); });

    auto varNameEnd = std::find_if(varNameStart, aStmt.begin() + aOpPos,
                                   [](char c) { return std::isspace(c); });

    // TODO Check whether the next token is the token of not. This handles the
    // case where a token has a white-space.
    return std::string_view(&*varNameStart, varNameEnd - varNameStart);
  }

  // Constructs the string_view of the token that extends between the position
  // of the operator at aStmt[aOpPos] and the end of the line represented by
  // aStmt;
  static std::string_view ExtractTokenViewAfter(const std::string &aStmt,
                                                std::size_t aOpPos) {
    auto valStart = std::find_if(aStmt.begin() + aOpPos + 1, aStmt.end(),
                                 [](char c) { return !std::isspace(c); });

    auto valEnd = std::find_if(valStart, aStmt.end(),
                               [](char c) { return std::isspace(c); });

    return std::string_view(&*valStart, valEnd - valStart);
  }
};

class VarDefStmt {
public:
  VarDefStmt(std::string &&aStmt, const std::string_view &aVarNameView,
             const std::string_view &aValView)
      : iStmt(aStmt), iVarNameView(aVarNameView), iValView(aValView) {
    std::cout << "Variable name: " << iVarNameView << std::endl;
    std::cout << "Value: " << iValView << std::endl;
  }

private:
  std::string iStmt;
  std::string_view iVarNameView;
  std::string_view iValView;
};

class VarDefStmtParser {
public:
  VarDefStmt operator()(std::string &&aStmt) {
    auto asgnPos = aStmt.find('=');

    if (asgnPos == 0 || asgnPos == std::string::npos) {
      throw std::invalid_argument(
          "Syntax error. Invalid assignment statement: " + aStmt);
    }

    return {std::move(aStmt), parseVarName(aStmt, asgnPos),
            parseVal(aStmt, asgnPos)};
  }

private:
  std::string_view parseVarName(const std::string &aStmt,
                                std::size_t aAsgnPos) {
    // TODO Add syntactic checking for valid variable names.
    return ParseUtils::ExtractTokenViewBefore(aStmt, aAsgnPos);
  }

  std::string_view parseVal(const std::string &aStmt, std::size_t aAsgnPos) {
    // TODO Add syntactic checking for valid integer values.
    return ParseUtils::ExtractTokenViewAfter(aStmt, aAsgnPos);
  }
};

class SrcFileParser {
public:
  void operator()(std::ifstream &&aIn) {
    constexpr size_t BUF_SIZE = 256;
    char buf[BUF_SIZE];

    while (aIn.getline(buf, BUF_SIZE)) {
      std::string line(buf);
      std::cout << "-- " << line << std::endl;
      VarDefStmtParser()(std::move(line));
    }
  }
};

int main(int argc, char *argv[]) {
  SrcFileParser p;
  p(std::ifstream(argv[1]));

  return 0;
}
