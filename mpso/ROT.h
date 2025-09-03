#pragma once

#include <cryptoTools/Network/Channel.h>
#include <cryptoTools/Common/BitVector.h>
#include <cryptoTools/Network/Session.h>
#include <cryptoTools/Network/IOService.h>

#include <libOTe/Base/BaseOT.h>
#include <libOTe/TwoChooseOne/SoftSpokenOT/SoftSpokenShOtExt.h>
#include <coproto/Socket/AsioSocket.h>
#include <iostream>
#include <volePSI/config.h>
#include "../common/Defines.h"

using namespace oc;

// SoftSpoken random oblivious transfer
void SoftSend(u32 numElements, Socket &chl, PRNG& prng, AlignedVector<std::array<block, 2>> &sMsgs, u32 numThreads = 1);
void SoftRecv(u32 numElements, BitVector bitV, Socket &chl, PRNG& prng, AlignedVector<block> &rMsgs, u32 numThreads = 1);


