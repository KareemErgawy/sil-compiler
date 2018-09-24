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
    auto assignPos = aStmt.find('=');

    if (assignPos == 0 || assignPos == std::string::npos) {
      throw std::invalid_argument(
          "Syntax error. Invalid assignment statement: " + aStmt);
    }

    return {std::move(aStmt),
            ParseUtils::ExtractTokenViewBefore(aStmt, assignPos),
            ParseUtils::ExtractTokenViewAfter(aStmt, assignPos)};
  }
};

class AddStmt {
public:
  AddStmt(std::string &&aStmt, const std::string_view &aAssignedVarView,
          const std::string_view &aLHSVarView,
          const std::string_view &aRHSVarView)
      : iStmt(aStmt), iAssignedVarView(aAssignedVarView),
        iLHSVarView(aLHSVarView), iRHSVarView(aRHSVarView) {
    std::cout << "Assigned variable name: " << iAssignedVarView << std::endl;
    std::cout << "LHS variable name: " << iLHSVarView << std::endl;
    std::cout << "RHS variable name: " << iRHSVarView << std::endl;
  }

private:
  std::string iStmt;
  std::string_view iAssignedVarView;
  std::string_view iLHSVarView;
  std::string_view iRHSVarView;
};

class AddStmtParser {
public:
  AddStmt operator()(std::string &&aStmt) {
    auto assignPos = aStmt.find('=');
    auto plusPos = aStmt.find('+');

    return {std::move(aStmt),
            ParseUtils::ExtractTokenViewBefore(aStmt, assignPos),
            ParseUtils::ExtractTokenViewBetween(aStmt, assignPos, plusPos),
            ParseUtils::ExtractTokenViewAfter(aStmt, plusPos)};
  }
};

class SrcFileParser {
public:
  void operator()(std::ifstream &&aIn) {
    constexpr size_t BUF_SIZE = 256;
    char buf[BUF_SIZE];
    VarDefStmtParser varDefParser;
    AddStmtParser addParser;

    while (aIn.getline(buf, BUF_SIZE)) {
      std::string line(buf);
      std::cout << "-- " << line << std::endl;
      if (line.find('+') != std::string::npos) {
        addParser(std::move(line));
      } else {
        varDefParser(std::move(line));
      }
    }
  }
};

int main(int argc, char *argv[]) {
  SrcFileParser p;
  p(std::ifstream(argv[1]));

  return 0;
}
