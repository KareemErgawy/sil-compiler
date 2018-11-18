#include <stdio.h>

int scheme_entry();

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
const unsigned int CharMask = 0xFF;
const unsigned int CharTag = 0x0F;

typedef unsigned int ptr;

static void print_char(char c) {
  if (c == ' ') {
    printf("#\\\\space");
  } else if (c == '\n') {
    printf("#\\\\newline");
  } else if (c == '\t') {
    printf("#\\\\tab");
  } else if (c == '\r') {
    printf("#\\\\return");
  } else if (c == '"') {
    printf("#\\\\\\\"");
  } else if (c == '\\') {
    printf("#\\\\\\\\");
  } else {
    printf("#\\\\%c", c);
  }
}

static void print_ptr(ptr x) {
  if ((x & FxMask) == FxTag) {
    printf("%d", ((int)x) >> FxShift);
  } else if (x == BoolF) {
    printf("#f");
  } else if (x == BoolT) {
    printf("#t");
  } else if (x == Null) {
    printf("()");
  } else if ((x & CharMask) == CharTag) {
    print_char((char)(((int)x) >> CharShift));
  } else {
    printf("#<unknown 0x%08x>", x);
  }

  printf("\n");
}

int main(int argc, char **argv) {
  print_ptr(scheme_entry());
  return 0;
}
