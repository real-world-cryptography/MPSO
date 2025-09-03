#pragma once

#include <iostream>
#include <volePSI/config.h>
#include <volePSI/Paxos.h>

#include "../offlineGen/BeaverTriplesGen.h"
#include "../shuffle/ShareCorrelationGen.h"
#include "../shuffle/MShuffle.h"
#include "PMT.h"
using namespace oc;

std::vector<u64> MPSIParty(u32 idx, u32 numParties, u32 numElements, std::vector<block> &set, u32 numThreads = 1);
