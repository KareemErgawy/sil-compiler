#include <iostream>
#include <sstream>
#include <string>
#include <string_view>

class IStmtAST {
protected:
  IStmtAST(const IStmtAST &) = delete;

  IStmtAST(std::string &&aStmt) : iStmt(aStmt) {}

public:
  virtual ~IStmtAST() = default;

  virtual std::string Dump() const = 0;

protected:
  std::string iStmt;
};

class VarDefStmt : public IStmtAST {
public:
  VarDefStmt(const VarDefStmt &) = delete;

  VarDefStmt(VarDefStmt &&aVarDefStmt)
      : IStmtAST(std::move(aVarDefStmt.iStmt)),
        iVarNameView(aVarDefStmt.iVarNameView), iValView(aVarDefStmt.iValView) {
  }

  VarDefStmt(std::string &&aStmt, const std::string_view &aVarNameView,
             const std::string_view &aValView)
      : IStmtAST(std::move(aStmt)), iVarNameView(aVarNameView),
        iValView(aValView) {
    std::stringstream ss;
    ss << "VarDefStmt [ defines variable: " << iVarNameView
       << " with value: " << iValView << " ]";
    std::cout << ss.str() << "\n";
  }

  ~VarDefStmt() = default;

  std::string Dump() const override {
    std::stringstream ss;
    ss << "VarDefStmt [ defines variable: " << iVarNameView
       << " with value: " << iValView << " ]";

    return ss.str();
  }

private:
  std::string_view iVarNameView;
  std::string_view iValView;
};

class AddStmt : public IStmtAST {
public:
  AddStmt(const AddStmt &) = delete;

  AddStmt(AddStmt &&aAddStmt)
      : IStmtAST(std::move(aAddStmt.iStmt)), iLHSVarView(aAddStmt.iLHSVarView),
        iRHSVarView(aAddStmt.iRHSVarView) {}

  AddStmt(std::string &&aStmt, const std::string_view &aAssignedVarView,
          const std::string_view &aLHSVarView,
          const std::string_view &aRHSVarView)
      : IStmtAST(std::move(aStmt)), iAssignedVarView(aAssignedVarView),
        iLHSVarView(aLHSVarView), iRHSVarView(aRHSVarView) {
    std::stringstream ss;
    ss << "AddStmt [ assigns variable: " << iAssignedVarView
       << " with: " << iLHSVarView << " + " << iRHSVarView << " ]";
    std::cout << ss.str() << "\n";
  }

  ~AddStmt() = default;

  std::string Dump() const override {
    std::stringstream ss;
    ss << "AddStmt [ assigns variable: " << iAssignedVarView
       << " with: " << iLHSVarView << " + " << iRHSVarView << " ]";

    return ss.str();
  }

private:
  std::string_view iAssignedVarView;
  std::string_view iLHSVarView;
  std::string_view iRHSVarView;
};
