#ifndef SYMBOL_TABLE
#define SYMBOL_TABLE

#include <iostream>
#include <unordered_map>

class SymbolTable {
public:
  SymbolTable(const StmtASTList &astList) {
    for (const auto &stmt : astList) {
      if (stmt->GetStmtType() == StmtType::VarDef) {
        const auto &varDefStmt = static_cast<VarDefStmt &>(*stmt);
        varToValMap[varDefStmt.iVarNameView] = std::stoi(varDefStmt.iValView);
        std::cout << "Mapped: " << varDefStmt.iVarNameView << " to "
                  << varToValMap[varDefStmt.iVarNameView] << "\n";
      }
    }
  }

private:
  std::unordered_map<std::string, int64_t> varToValMap;
};

#endif
