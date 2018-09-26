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
        varToValMap[varDefStmt.iVarName] = std::stoi(varDefStmt.iVal);
        std::cout << "Mapped: " << varDefStmt.iVarName << " to "
                  << varToValMap[varDefStmt.iVarName] << "\n";
      }
    }
  }

private:
  std::unordered_map<std::string, int64_t> varToValMap;
};

#endif
