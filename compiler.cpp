#include "parser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

int main(int argc, char *argv[]) {
  SrcFileParser p;
  auto result = p(std::ifstream(argv[1]));

  for (const auto &stmt : result) {
    std::cout << stmt->Dump() << std::endl;
  }

  return 0;
}
