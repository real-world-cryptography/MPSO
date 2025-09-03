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

u64 gf64Mul(u64 &x, u64 &y);

void drityBeaverGen(u32 totalParties, u32 numParties, u32 logNum, std::string postfixPath = "");
// for MPSU
void beaverGenMPSU(u32 totalParties, u32 logNum);

// for MPSI
void beaverGenMPSI(u32 totalParties, u32 logNum);
void beaverTripleMulOnline(u32 idx, u32 currentParties, std::vector<block> &vecInput, std::vector<Socket> &chl, std::vector<block> &vecOut, std::string postfixPath);


void drityBeaverGen64(u32 totalParties, u32 numParties, u32 logNum, std::string postfixPath = "");
void beaverGenMPSI64(u32 totalParties, u32 logNum);
void beaverTripleMulOnline64(u32 idx, u32 currentParties, std::vector<u64> &vecInput, std::vector<Socket> &chl, std::vector<u64> &vecOut, std::string postfixPath);


