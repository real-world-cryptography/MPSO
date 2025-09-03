#pragma once
#include <string> 
#include <iostream>
#include <fstream>

#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>

#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/SoftSpokenOT/SoftSpokenShOtExt.h>
#include <coproto/Socket/AsioSocket.h>
#include <volePSI/config.h>

#include <cryptoTools/Common/CuckooIndex.h>
#include "../common/Defines.h"

#include "../mpso/ROT.h"
using namespace oc;


// for our MPSU
void rotGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads = 1);
// for dml MPSU5 if myIdx < idx, we use sender  
void rot2PartyGenMPSU(u32 isRecv, u32 numBins, Socket &chl, std::string fileName, u32 numThreads = 1);




