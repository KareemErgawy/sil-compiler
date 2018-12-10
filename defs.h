#ifndef DEFS_H
#define DEFS_H

#include <cmath>
#include <string>
#include <unordered_map>
#include <vector>

using TBindings = std::unordered_map<std::string, std::string>;
using TOrderedBindings = std::vector<std::pair<std::string, std::string>>;
using TEnvironment = std::unordered_map<std::string, int>;
using TLambdaTable = std::unordered_map<std::string, std::string>;

using TUnaryPrimitiveEmitter = std::string (*)(int, TEnvironment, std::string,
                                               bool);
using TBinaryPrimitiveEmitter = std::string (*)(int, TEnvironment, std::string,
                                                std::string, bool);
using TTernaryPrimitiveEmitter = std::string (*)(int, TEnvironment, std::string,
                                                 std::string, std::string,
                                                 bool);

const unsigned int FxShift = 2;
const unsigned int FxMask = 0x03;
const int FxMaskNeg = 0xFFFFFFFC;
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

const unsigned int PairMask = 0x07;
const unsigned int PairTag = 0x01;

const unsigned int VectorMask = 0x07;
const unsigned int VectorTag = 0x05;

const unsigned int StringMask = 0x07;
const unsigned int StringTag = 0x06;

const int WordSize = 8;
const int WordSizeLg2 = 3;

const int FixNumBits = WordSize * 8 - FxShift;
const long FxLower = -pow(2, FixNumBits - 1);
const long FxUpper = pow(2, FixNumBits - 1) - 1;

#endif
