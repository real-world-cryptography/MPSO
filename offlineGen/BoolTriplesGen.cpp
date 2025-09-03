#include "BoolTriplesGen.h"


// for dml
void twoPartyTripleGenMPSU(u32 isLarger, u32 numParties, u32 numElements, Socket &chl, std::string outFileName, u32 numThreads)
{

    std::ofstream outFile;
    outFile.open(outFileName, std::ios::binary | std::ios::out);
    if (!outFile.is_open()){
        std::cout << "Error opening file " << outFileName << std::endl;
        return;
    }

    u32 ssp = 40;
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    u32 interactNum = (numParties * (numParties-1))/2;
    u32 keyBitLength = ssp + oc::log2ceil(numBins * interactNum);  

    volePSI::BetaCircuit cir = volePSI::isZeroCircuit(keyBitLength);
    // generate triples

    volePSI::Gmw gmw;
    gmw.init(numBins, cir, numThreads, isLarger, sysRandomSeed());
    coproto::sync_wait(gmw.generateTriple(1 << 24, numThreads, chl));
    // write A, B, C, D
    outFile.write((char*)gmw.mA.data(), gmw.mA.size() * sizeof(block));
    outFile.write((char*)gmw.mB.data(), gmw.mB.size() * sizeof(block));
    outFile.write((char*)gmw.mC.data(), gmw.mC.size() * sizeof(block));
    outFile.write((char*)gmw.mD.data(), gmw.mD.size() * sizeof(block));

    outFile.close();
    coproto::sync_wait(chl.flush());

}


void boolTriplesGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads){
    u32 numElements = 1ull << logNum;
    // connect
    std::vector<Socket> chl;
    for (u32 i = 0; i < numParties; ++i){
        // PORT + senderId * 100 + receiverId
        if (i < idx){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + i), true));
        } else if (i > idx){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    }

    // generate triples
    std::vector<std::thread> tripleGenThrds(numParties - 1);
    for (u32 i = 0; i < numParties; ++i){
        std::string fileName = "./offline/triple_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1);
        if (i < idx){
            tripleGenThrds[i] = std::thread([&, i, fileName]() {
                twoPartyTripleGenMPSU(1, numParties, numElements, chl[i], fileName, numThreads);
            });
        } else if (i > idx){
            tripleGenThrds[i - 1] = std::thread([&, i, fileName]() {
                twoPartyTripleGenMPSU(0, numParties, numElements, chl[i - 1], fileName, numThreads);
            });
        }
    }
    for (auto& thrd : tripleGenThrds) thrd.join();    

    // close channel
    for (u32 i = 0; i < chl.size(); ++i){
        coproto::sync_wait(chl[i].flush());
        coproto::sync_wait(chl[i].close());;
    }

}



// void boolTriplesGenMPSU(u32 idx, u32 numParties, u32 logNum, std::vector<Socket> &chl, u32 numThreads){
//     u32 numElements = 1ull << logNum;

//     // generate triples
//     std::vector<std::thread> tripleGenThrds(numParties - 1);
//     for (u32 i = 0; i < numParties; ++i){
//         std::string fileName = "./offline/triple_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1);
//         if (i < idx){
//             tripleGenThrds[i] = std::thread([&, i, fileName]() {
//                 twoPartyTripleGenMPSU(1, numParties, numElements, chl[i], fileName, numThreads);
//             });
//         } else if (i > idx){
//             tripleGenThrds[i - 1] = std::thread([&, i, fileName]() {
//                 twoPartyTripleGenMPSU(0, numParties, numElements, chl[i - 1], fileName, numThreads);
//             });
//         }
//     }
//     for (auto& thrd : tripleGenThrds) thrd.join();    

//     // flush channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush());
//     }

// }



