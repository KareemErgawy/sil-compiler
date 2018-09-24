#include <iostream>
#include <string_view>

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


