#pragma once
#include <string> 
#include <iostream>
#include <fstream>
#include <libOTe/Vole/Silent/SilentVoleReceiver.h>
#include <libOTe/Vole/Silent/SilentVoleSender.h>
#include <coproto/Socket/AsioSocket.h>

#include <cryptoTools/Common/CuckooIndex.h>
#include "../common/Defines.h"

using namespace oc;

void voleGenSend(u32 myIdx, u32 logNum, u32 numThreads, Socket &chl, std::string fileName);

void voleGenRecv(u32 myIdx, u32 logNum, u32 numThreads, Socket &chl, std::string fileName);
//for MPSU
void voleGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads);

// for PSI
void voleGenMPSI(u32 idx, u32 numParties, u32 logNum, u32 numThreads);

// for PSICS
void voleGenMPSICS(u32 idx, u32 numParties, u32 logNum,  u32 numThreads);



