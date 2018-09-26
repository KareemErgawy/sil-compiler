#ifndef AST_H
#define AST_H

#include <sstream>
#include <string>

class IStmtAST {
public:
  IStmtAST(std::string &aStmt) : iStmt(aStmt) {}

  virtual ~IStmtAST() = default;

  virtual std::string Dump() const = 0;

protected:
  std::string iStmt;
};

class VarDefStmt : public IStmtAST {
public:
  VarDefStmt(std::string &aStmt, const std::string &aVarNameView,
             const std::string &aValView)
      : IStmtAST(aStmt), iVarNameView(aVarNameView), iValView(aValView) {}

  std::string Dump() const override {
    std::stringstream ss;
    ss << "VarDefStmt [ defines variable: " << iVarNameView
       << " with value: " << iValView << " ]";

    return ss.str();
  }

private:
  std::string iVarNameView;
  std::string iValView;
};

class AddStmt : public IStmtAST {
public:
  AddStmt(std::string &aStmt, const std::string &aAssignedVarView,
          const std::string &aLHSVarView, const std::string &aRHSVarView)
      : IStmtAST(aStmt), iAssignedVarView(aAssignedVarView),
        iLHSVarView(aLHSVarView), iRHSVarView(aRHSVarView) {}

  std::string Dump() const override {
    std::stringstream ss;
    ss << "AddStmt [ assigns variable: " << iAssignedVarView
       << " with: " << iLHSVarView << " + " << iRHSVarView << " ]";

    return ss.str();
  }

private:
  std::string iAssignedVarView;
  std::string iLHSVarView;
  std::string iRHSVarView;
};

#endif
