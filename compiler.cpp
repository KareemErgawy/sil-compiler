#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

class ParseUtils {
public:
  //! Constructs the string_view of the token that extends between
  //! aStmt[aPos1+1] and aStmt[aPos2-1].
  static std::string_view ExtractTokenViewBetween(const std::string &aStmt,
                                                  std::size_t aPos1,
                                                  std::size_t aPos2) {
    auto tokenStart =
        std::find_if(aStmt.begin() + aPos1 + 1, aStmt.begin() + aPos2,
                     [](char c) { return !std::isspace(c); });

    auto tokenEnd = std::find_if(tokenStart, aStmt.begin() + aPos2,
                                 [](char c) { return std::isspace(c); });

    return std::string_view(&*tokenStart, tokenEnd - tokenStart);
  }

  //! Constructs the string_view of the token that extends between
  //! aStmt[0] and aStmt[aPos-1].
  static std::string_view ExtractTokenViewBefore(const std::string &aStmt,
                                                 std::size_t aPos) {
    return ExtractTokenViewBetween(aStmt, -1, aPos);
  }

  //! Constructs the string_view of the token that extends between
  //! aStmt[aPos] and aStmt[aStmt.size() - 1].
  static std::string_view ExtractTokenViewAfter(const std::string &aStmt,
                                                std::size_t aPos) {
    return ExtractTokenViewBetween(aStmt, aPos, aStmt.size());
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
