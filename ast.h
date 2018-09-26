#ifndef AST_H
#define AST_H

#include <sstream>
#include <string>

enum class StmtType { VarDef, Add };

class IStmtAST {
public:
  IStmtAST(std::string &aStmt) : iStmt(aStmt) {}

  virtual ~IStmtAST() = default;

  virtual std::string Dump() const = 0;

  virtual StmtType GetStmtType() const = 0;

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

  StmtType GetStmtType() const override { return StmtType::VarDef; }

  const std::string iVarNameView;
  const std::string iValView;
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

  StmtType GetStmtType() const override { return StmtType::Add; }

  const std::string iAssignedVarView;
  const std::string iLHSVarView;
  const std::string iRHSVarView;
};

#endif
