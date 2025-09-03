
#include "BeaverTriplesGen.h"


u64 gf64Mul(u64 &x, u64 &y){
    block x128 = _mm_loadl_epi64((const __m128i*) & x);
    block y128 = _mm_loadl_epi64((const __m128i*) & y);
    constexpr u64 gf64mod = (1 << 4) | (1 << 3) | (1 << 1) | 1;
    block mod = toBlock(gf64mod);

    block xMULy = _mm_clmulepi64_si128(x128, y128, (int)0x00);

    block mul = xMULy.clmulepi64_si128<0x01>(mod);

    // Output can include 4 bits in the high half. Multiply again (producing an 8 bit result) to
    // reduce these into the low 64 bits.
    block reduced = mul.clmulepi64_si128<0x01>(mod);
    reduced ^= mul ^ xMULy;
    return reduced.get<u64>()[0];
}




// for MPSU
void beaverGenMPSU(u32 totalParties, u32 logNum){
    for (u32 j = 2; j < totalParties + 1; ++j){
        drityBeaverGen(totalParties, j, logNum);
    }
}

// for MPSI
void beaverGenMPSI(u32 totalParties, u32 logNum)
{
    drityBeaverGen(totalParties, totalParties, logNum, "PSI");
}


void drityBeaverGen(u32 totalParties, u32 currentParties, u32 logNum, std::string postfixPath){
    u32 numElements = 1ull << logNum;
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    PRNG prng(sysRandomSeed());

    std::vector<std::vector<block>> vvecA(currentParties, std::vector<block>(numBins, block(0,0)));
    std::vector<std::vector<block>> vvecB(currentParties, std::vector<block>(numBins, block(0,0)));
    std::vector<std::vector<block>> vvecC(currentParties, std::vector<block>(numBins, block(0,0)));
    for (u32 i = 0; i < currentParties; ++i){
        prng.get(vvecA[i].data(), numBins);
        prng.get(vvecB[i].data(), numBins);
    }

    for (u32 i = 0; i < currentParties-1; ++i){
        prng.get(vvecC[i].data(), numBins);
    }    

    // compute sumA, sumB, C[currentParties-1] = sumC ^ (sumA * sumB)
    std::vector<block> sumA(numBins);
    std::vector<block> sumB(numBins);
    std::vector<block> sumC(numBins);

    for (u32 i = 0; i < numBins; ++i){
        for (u32 j = 0; j < currentParties; ++j){
            sumA[i] ^= vvecA[j][i];
            sumB[i] ^= vvecB[j][i];
        }
    }
    for (u32 i = 0; i < numBins; ++i){
        for (u32 j = 0; j < currentParties-1; ++j){
            sumC[i] ^= vvecC[j][i];
        }
    }   

    for (u32 i = 0; i < numBins; ++i){
        vvecC[currentParties-1][i] = (sumA[i].gf128Mul(sumB[i])) ^ sumC[i];
    }

    std::string outFileName;
    std::ofstream outFile;
    for (u32 i = 0; i < currentParties; ++i){
        if(postfixPath.empty()){
            outFileName = "./offline/bt_" + std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(currentParties); 
        }  
        else{
            outFileName = "./offline/bt_" + std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(currentParties) + postfixPath; 
        }      

        outFile.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFile.is_open()){
            std::cout << "Vole error opening file " << outFileName << std::endl;
            return;
        }

        outFile.write((char*)vvecA[i].data(), numBins * sizeof(block));
        outFile.write((char*)vvecB[i].data(), numBins * sizeof(block));
        outFile.write((char*)vvecC[i].data(), numBins * sizeof(block));

        outFile.close();
        outFileName.clear();
    }
}


/*
0, x, a, b
1, y, c, d

a*c = b^d, how to get z0 ^ z1 = x*y

idx0: send (a ^ x) to idx1
idx1: send (y ^ c) to idx0

idx0: compute z0 = (y ^ c)*x -b = x*y ^ c*x ^ b
idx1: compute z1 = (a ^ x)*c ^ d =  c*x ^ (a*c ^ d) = c*x ^ b

*/
void beaverTripleMulOnline(u32 idx, u32 currentParties, std::vector<block> &vecInput, std::vector<Socket> &chl, std::vector<block> &vecOut, std::string postfixPath)
{
    u32 numBins = vecInput.size();
    vecOut.resize(numBins);
    std::vector<block> vecA(numBins);
    std::vector<block> vecB(numBins);
    std::vector<block> vecC(numBins);

    // read corresponding vecA, vecB, vecC from files
    std::string beaverPath = "./offline/bt_" + postfixPath;
    std::ifstream beaverFile;
    beaverFile.open(beaverPath, std::ios::binary | std::ios::in);
    if (!beaverFile.is_open()){
        std::cout << "Error opening file " << beaverPath << std::endl;
        return;
    }    
    beaverFile.read((char*)vecA.data(), vecA.size() * sizeof(block)); 
    beaverFile.read((char*)vecB.data(), vecB.size() * sizeof(block));
    beaverFile.read((char*)vecC.data(), vecC.size() * sizeof(block));
    beaverFile.close();

    std::vector<block> vecSaddA(numBins);
    for (u32 i = 0; i < numBins; ++i){
        vecSaddA[i] = vecInput[i] ^ vecA[i];
    }

    std::vector<block> vecSumA(numBins);
    // recv all the other vecSaddA, compute vecSumA, send vecSumA to other parties
    if(idx == 0){
        std::vector<std::vector<block>> vvecSA(currentParties-1, std::vector<block>(numBins, block(0,0)));    
        for (u32 i = 0; i < currentParties-1; ++i){
            coproto::sync_wait(chl[i].recv(vvecSA[i]));
        }

        for (u32 i = 0; i < numBins; ++i){
            vecSumA[i] = vecSaddA[i];
            for (u32 j = 0; j < currentParties-1; ++j){
                vecSumA[i] ^= vvecSA[j][i];
            }
        }
        for (u32 i = 0; i < currentParties-1; ++i){
            coproto::sync_wait(chl[i].send(vecSumA));
        }
    }else if(idx < currentParties){
        coproto::sync_wait(chl[0].send(vecSaddA));
        coproto::sync_wait(chl[0].recv(vecSumA));
    }

    for (u32 i = 0; i < numBins; ++i){
        vecOut[i] = vecSumA[i].gf128Mul(vecB[i]) ^ vecC[i];
    }

}




void beaverGenMPSI64(u32 totalParties, u32 logNum)
{
    drityBeaverGen64(totalParties, totalParties, logNum, "PSI64");
}


void drityBeaverGen64(u32 totalParties, u32 currentParties, u32 logNum, std::string postfixPath){
    u32 numElements = 1ull << logNum;
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    PRNG prng(sysRandomSeed());

    std::vector<std::vector<u64>> vvecA(currentParties, std::vector<u64>(numBins, 0));
    std::vector<std::vector<u64>> vvecB(currentParties, std::vector<u64>(numBins, 0));
    std::vector<std::vector<u64>> vvecC(currentParties, std::vector<u64>(numBins, 0));
    for (u32 i = 0; i < currentParties; ++i){
        prng.get(vvecA[i].data(), numBins);
        prng.get(vvecB[i].data(), numBins);
    }

    for (u32 i = 0; i < currentParties-1; ++i){
        prng.get(vvecC[i].data(), numBins);
    }    

    // compute sumA, sumB, C[currentParties-1] = sumC ^ (sumA * sumB)
    std::vector<u64> sumA(numBins);
    std::vector<u64> sumB(numBins);
    std::vector<u64> sumC(numBins);

    for (u32 i = 0; i < numBins; ++i){
        for (u32 j = 0; j < currentParties; ++j){
            sumA[i] ^= vvecA[j][i];
            sumB[i] ^= vvecB[j][i];
        }
    }
    for (u32 i = 0; i < numBins; ++i){
        for (u32 j = 0; j < currentParties-1; ++j){
            sumC[i] ^= vvecC[j][i];
        }
    }   

    for (u32 i = 0; i < numBins; ++i){
        vvecC[currentParties-1][i] = gf64Mul(sumA[i], sumB[i]) ^ sumC[i];
    }

    std::string outFileName;
    std::ofstream outFile;
    for (u32 i = 0; i < currentParties; ++i){
        if(postfixPath.empty()){
            outFileName = "./offline/bt_" + std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(currentParties); 
        }  
        else{
            outFileName = "./offline/bt_" + std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(currentParties) + postfixPath; 
        }      

        outFile.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFile.is_open()){
            std::cout << "Vole error opening file " << outFileName << std::endl;
            return;
        }

        outFile.write((char*)vvecA[i].data(), numBins * sizeof(u64));
        outFile.write((char*)vvecB[i].data(), numBins * sizeof(u64));
        outFile.write((char*)vvecC[i].data(), numBins * sizeof(u64));

        outFile.close();
        outFileName.clear();
    }
}


void beaverTripleMulOnline64(u32 idx, u32 currentParties, std::vector<u64> &vecInput, std::vector<Socket> &chl, std::vector<u64> &vecOut, std::string postfixPath)
{
    u32 numBins = vecInput.size();
    vecOut.resize(numBins);
    std::vector<u64> vecA(numBins);
    std::vector<u64> vecB(numBins);
    std::vector<u64> vecC(numBins);

    // read corresponding vecA, vecB, vecC from files
    std::string beaverPath = "./offline/bt_" + postfixPath;
    std::ifstream beaverFile;
    beaverFile.open(beaverPath, std::ios::binary | std::ios::in);
    if (!beaverFile.is_open()){
        std::cout << "Error opening file " << beaverPath << std::endl;
        return;
    }    
    beaverFile.read((char*)vecA.data(), vecA.size() * sizeof(u64)); 
    beaverFile.read((char*)vecB.data(), vecB.size() * sizeof(u64));
    beaverFile.read((char*)vecC.data(), vecC.size() * sizeof(u64));
    beaverFile.close();

    std::vector<u64> vecSaddA(numBins);
    for (u32 i = 0; i < numBins; ++i){
        vecSaddA[i] = vecInput[i] ^ vecA[i];
    }

    std::vector<u64> vecSumA(numBins);
    // recv all the other vecSaddA, compute vecSumA, send vecSumA to other parties
    if(idx == 0){
        std::vector<std::vector<u64>> vvecSA(currentParties-1, std::vector<u64>(numBins, 0));    
        for (u32 i = 0; i < currentParties-1; ++i){
            coproto::sync_wait(chl[i].recv(vvecSA[i]));
        }

        for (u32 i = 0; i < numBins; ++i){
            vecSumA[i] = vecSaddA[i];
            for (u32 j = 0; j < currentParties-1; ++j){
                vecSumA[i] ^= vvecSA[j][i];
            }
        }
        for (u32 i = 0; i < currentParties-1; ++i){
            coproto::sync_wait(chl[i].send(vecSumA));
        }
    }else if(idx < currentParties){
        coproto::sync_wait(chl[0].send(vecSaddA));
        coproto::sync_wait(chl[0].recv(vecSumA));
    }

    for (u32 i = 0; i < numBins; ++i){
        vecOut[i] = gf64Mul(vecSumA[i], vecB[i]) ^ vecC[i];
    }

}



