#ifndef PARSER_H
#define PARSER_H

#include "ast.h"
#include "defs.h"

#include <algorithm>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

class ParseUtils {
public:
  //! Constructs the string of the token that extends between
  //! aStmt[aPos1+1] and aStmt[aPos2-1].
  static std::string ExtractTokenBetween(const std::string &aStmt,
                                         std::size_t aPos1, std::size_t aPos2) {
    auto tokenStart =
        std::find_if(aStmt.begin() + aPos1 + 1, aStmt.begin() + aPos2,
                     [](char c) { return !std::isspace(c); });

    auto tokenEnd = std::find_if(tokenStart, aStmt.begin() + aPos2,
                                 [](char c) { return std::isspace(c); });

    return std::string(&*tokenStart, tokenEnd - tokenStart);
  }

  //! Constructs the string of the token that extends between
  //! aStmt[0] and aStmt[aPos-1].
  static std::string ExtractTokenBefore(const std::string &aStmt,
                                        std::size_t aPos) {
    return ExtractTokenBetween(aStmt, -1, aPos);
  }

  //! Constructs the string of the token that extends between
  //! aStmt[aPos] and aStmt[aStmt.size() - 1].
  static std::string ExtractTokenAfter(const std::string &aStmt,
                                       std::size_t aPos) {
    return ExtractTokenBetween(aStmt, aPos, aStmt.size());
  }
};

class VarDefStmtParser {
public:
  VarDefStmt operator()(std::string &aStmt) {
    auto assignPos = aStmt.find('=');

    if (assignPos == 0 || assignPos == std::string::npos) {
      throw std::invalid_argument(
          "Syntax error. Invalid assignment statement: " + aStmt);
    }

    return {aStmt, ParseUtils::ExtractTokenBefore(aStmt, assignPos),
            ParseUtils::ExtractTokenAfter(aStmt, assignPos)};
  }
};

class AddStmtParser {
public:
  AddStmt operator()(std::string &aStmt) {
    auto assignPos = aStmt.find('=');
    auto plusPos = aStmt.find('+');

    return {aStmt, ParseUtils::ExtractTokenBefore(aStmt, assignPos),
            ParseUtils::ExtractTokenBetween(aStmt, assignPos, plusPos),
            ParseUtils::ExtractTokenAfter(aStmt, plusPos)};
  }
};

class SrcFileParser {
public:
  StmtASTList operator()(std::ifstream &&aIn) {
    StmtASTList result;
    constexpr size_t BUF_SIZE = 256;
    char buf[BUF_SIZE];
    VarDefStmtParser varDefParser;
    AddStmtParser addParser;

    while (aIn.getline(buf, BUF_SIZE)) {
      std::string line(buf);
      if (line.find('+') != std::string::npos) {
        result.push_back(std::make_unique<AddStmt>(addParser(line)));
      } else {
        result.push_back(std::make_unique<VarDefStmt>(varDefParser(line)));
      }
    }

    return result;
  }
};

#endif
