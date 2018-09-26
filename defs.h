#ifndef DEFS_H
#define DEFS_H

#include <memory>
#include <vector>

class IStmtAST;

using StmtASTList = std::vector<std::unique_ptr<IStmtAST>>;

#endif
