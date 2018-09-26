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
  VarDefStmt(std::string &aStmt, const std::string &aVarName,
             const std::string &aVal)
      : IStmtAST(aStmt), iVarName(aVarName), iVal(aVal) {}

  std::string Dump() const override {
    std::stringstream ss;
    ss << "VarDefStmt [ defines variable: " << iVarName
       << " with value: " << iVal << " ]";

    return ss.str();
  }

  StmtType GetStmtType() const override { return StmtType::VarDef; }

  const std::string iVarName;
  const std::string iVal;
};

class AddStmt : public IStmtAST {
public:
  AddStmt(std::string &aStmt, const std::string &aAssignedVar,
          const std::string &aLHSVar, const std::string &aRHSVar)
      : IStmtAST(aStmt), iAssignedVar(aAssignedVar), iLHSVar(aLHSVar),
        iRHSVar(aRHSVar) {}

  std::string Dump() const override {
    std::stringstream ss;
    ss << "AddStmt [ assigns variable: " << iAssignedVar << " with: " << iLHSVar
       << " + " << iRHSVar << " ]";

    return ss.str();
  }

  StmtType GetStmtType() const override { return StmtType::Add; }

  const std::string iAssignedVar;
  const std::string iLHSVar;
  const std::string iRHSVar;
};

#endif
