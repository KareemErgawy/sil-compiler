#ifndef DEFS_H
#define DEFS_H

#include <cmath>
#include <string>
#include <unordered_map>

using TBindings = std::unordered_map<std::string, std::string>;
using TEnvironment = std::unordered_map<std::string, int>;

using TUnaryPrimitiveEmitter = std::string (*)(int, TEnvironment, std::string);
using TBinaryPrimitiveEmitter = std::string (*)(int, TEnvironment, std::string,
                                                std::string);

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const unsigned int FxMaskNeg = 0xFFFFFFFC;
const unsigned int FxTag = 0x00;

const unsigned int BoolF = 0x2F;
const unsigned int BoolT = 0x6F;
const unsigned int BoolMask = 0x3F;
const unsigned int BoolTag = 0x2F;
const unsigned int BoolBit = 6;

const unsigned int Null = 0x3F;

const unsigned int CharShift = 8;
const unsigned int CharMask = 0xFF;
const unsigned int CharTag = 0x0F;

const unsigned int WordSize = 4;

const unsigned int FixNumBits = WordSize * 8 - FxShift;
const int FxLower = -pow(2, FixNumBits - 1);
const int FxUpper = pow(2, FixNumBits - 1) - 1;

#endif
