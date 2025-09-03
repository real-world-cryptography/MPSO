#pragma once
#include <iostream>
#include <fstream>
#include "volePSI/GMW/Gmw.h"
#include "coproto/Socket/AsioSocket.h"

#include "cryptoTools/Common/CuckooIndex.h"
#include "../common/Defines.h"


// for dml mpsu5
void twoPartyTripleGenMPSU(u32 isLarger, u32 numParties, u32 numElements, Socket &chl, std::string outFileName, u32 numThreads = 1);

void boolTriplesGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads = 1);




