#include "parser.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <string_view>
#include <utility>

int main(int argc, char *argv[]) {
  SrcFileParser p;
  p(std::ifstream(argv[1]));

  return 0;
}
