#pragma once

#include <cryptoTools/Common/CLP.h>
#include <iostream>
#include <bitset>
#include "Defines.h"

inline std::random_device rd2;
inline std::mt19937 global_built_in_prg2(rd2());


// permute data according to pi
void genPermutation(u32 size, std::vector<u32> &pi);

void permute64(std::vector<u32> &pi, std::vector<u64> &data);

void permute(std::vector<u32> &pi, std::vector<block> &data);
