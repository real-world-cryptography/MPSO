#include "VoleGen.h"

// simple, basic VOLE generate process: mA + mB = mC * mD
// get mB, mD
void voleGenSend(u32 myIdx, u32 numElements, u32 numThreads, Socket &chl, std::string outFileName)
{
    std::ofstream outFile;
    outFile.open(outFileName, std::ios::binary | std::ios::out);
    if (!outFile.is_open()){
        std::cout << "Vole error opening file " << outFileName << std::endl;
        return;
    }

    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    
    PRNG prng(sysRandomSeed());
    // get vole : a + b  = c * d
    block mD = prng.get();
    oc::SilentVoleSender<block,block, oc::CoeffCtxGF128> mVoleSender;
    mVoleSender.mMalType = SilentSecType::SemiHonest;
    mVoleSender.configure(numBins, SilentBaseType::Base);
    AlignedUnVector<block> mB(numBins);
    coproto::sync_wait(mVoleSender.silentSend(mD, mB, prng, chl));   
    
    coproto::sync_wait(chl.flush());  
        
    outFile.write((char*)(&mD), sizeof(block));
    outFile.write((char*)mB.data(), mB.size() * sizeof(block));
    
    outFile.close();
      
}

// get mA, mC
void voleGenRecv(u32 myIdx, u32 numElements, u32 numThreads, Socket &chl, std::string outFileName)
{
    std::ofstream outFile;
    outFile.open(outFileName, std::ios::binary | std::ios::out);
    if (!outFile.is_open()){
        std::cout << "Vole error opening file " << outFileName << std::endl;
        return;
    }

    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    
    PRNG prng(sysRandomSeed());
    // get mA mC of vole: a+b = c*d
    oc::SilentVoleReceiver<block, block, oc::CoeffCtxGF128> mVoleRecver;
    mVoleRecver.mMalType = SilentSecType::SemiHonest;
    mVoleRecver.configure(numBins, SilentBaseType::Base);
    AlignedUnVector<block> mA(numBins);
    AlignedUnVector<block> mC(numBins);
    coproto::sync_wait(mVoleRecver.silentReceive(mC, mA, prng, chl));
    
    coproto::sync_wait(chl.flush());   
    outFile.write((char*)mA.data(), mA.size() * sizeof(block));
    outFile.write((char*)mC.data(), mC.size() * sizeof(block));
    outFile.close();     
}



// for MPSU
void voleGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads){

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

    // generate vole
    u32 numElements = 1ull << logNum;
    std::vector<std::thread> voleGenThrds(numParties - 1);
    for (u32 i = 0; i < numParties; ++i){
        std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1);
        if (i < idx){
            voleGenThrds[i] = std::thread([&, i, fileName]() {
                voleGenRecv(idx, numElements, numThreads, chl[i], fileName);
            });
        } else if (i > idx){
            voleGenThrds[i - 1] = std::thread([&, i, fileName]() {
                voleGenSend(idx, numElements, numThreads, chl[i - 1], fileName);
            });
        }
    }
    for (auto& thrd : voleGenThrds) thrd.join();


    // close channel
    for (u32 i = 0; i < chl.size(); ++i){
        coproto::sync_wait(chl[i].flush());
        coproto::sync_wait(chl[i].close());;
    }
    return;

}


// for MPSI/MPSIC, where P0 cuckoo, get OPPRF values(A, C), voleRecv
void voleGenMPSI(u32 idx, u32 numParties, u32 logNum, u32 numThreads){

    // connect
    std::vector<Socket> chl;
    if(idx == 0){
        for (u32 i = 1; i < numParties; ++i){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    } else{
        chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
    }

    // generate vole
    u32 numElements = 1ull << logNum;
    if(idx == 0){
        std::vector<std::thread> voleGenThrds(numParties - 1);
        for (u32 i = 1; i < numParties; ++i){
            std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSI";
            voleGenThrds[i-1] = std::thread([&, i, fileName]() {
                voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName);
            });            
        }
        for (auto& thrd : voleGenThrds) thrd.join(); 
    }
    else{
        std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSI";
        voleGenSend(idx, numElements, numThreads, chl[0], fileName);
    }

    for (u32 i = 0; i < chl.size(); ++i){
        coproto::sync_wait(chl[i].flush());
        coproto::sync_wait(chl[i].close());;
    }

}




// for MPSCS, where P0 cuckoo, get OPPRF values(A, C), voleRecv
void voleGenMPSICS(u32 idx, u32 numParties, u32 logNum, u32 numThreads){

    // connect
    std::vector<Socket> chl;
    if(idx == 0){
        for (u32 i = 1; i < numParties; ++i){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    } else{
        chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
    }

    // generate vole
    u32 numElements = 1ull << logNum;
    if(idx == 0){
        std::vector<std::thread> voleGenThrds(numParties - 1);
        for (u32 i = 1; i < numParties; ++i){
            std::string fileName1 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS1";
            std::string fileName2 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS2";
            voleGenThrds[i-1] = std::thread([&, i, fileName1, fileName2]() {
                voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName1);
                voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName2);
            });            
        }
        for (auto& thrd : voleGenThrds) thrd.join();

    }
    else{
        std::string fileName1 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS1";
        std::string fileName2 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS2";
        voleGenSend(idx, numElements, numThreads, chl[0], fileName1);
        voleGenSend(idx, numElements, numThreads, chl[0], fileName2);
    }

    for (u32 i = 0; i < chl.size(); ++i){
        coproto::sync_wait(chl[i].flush());
        coproto::sync_wait(chl[i].close());;
    }
    return;
}



// // for MPSU
// void voleGenMPSU(u32 idx, u32 numParties, u32 logNum, std::vector<Socket> &chl, u32 numThreads){

//     // generate vole
//     u32 numElements = 1ull << logNum;
//     std::vector<std::thread> voleGenThrds(numParties - 1);
//     for (u32 i = 0; i < numParties; ++i){
//         std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1);
//         if (i < idx){
//             voleGenThrds[i] = std::thread([&, i, fileName]() {
//                 voleGenRecv(idx, numElements, numThreads, chl[i], fileName);
//             });
//         } else if (i > idx){
//             voleGenThrds[i - 1] = std::thread([&, i, fileName]() {
//                 voleGenSend(idx, numElements, numThreads, chl[i - 1], fileName);
//             });
//         }
//     }
//     for (auto& thrd : voleGenThrds) thrd.join();

//     // flush channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush()); 
//     }
//     return;

// }




// // for MPSI/MPSIC, where P0 cuckoo, get OPPRF values(A, C), voleRecv
// void voleGenMPSI(u32 idx, u32 numParties, u32 logNum, std::vector<Socket> &chl, u32 numThreads){

//     // generate vole
//     u32 numElements = 1ull << logNum;
//     if(idx == 0){
//         std::vector<std::thread> voleGenThrds(numParties - 1);
//         for (u32 i = 1; i < numParties; ++i){
//             std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSI";
//             voleGenThrds[i-1] = std::thread([&, i, fileName]() {
//                 voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName);
//             });            
//         }
//         for (auto& thrd : voleGenThrds) thrd.join();

//         // std::cout << "P1 vole done " << std::endl;  
//     }
//     else{
//         std::string fileName = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSI";
//         voleGenSend(idx, numElements, numThreads, chl[0], fileName);
//     }

//     // flush channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush()); 
//     }

// }


// // for MPSCS, where P0 cuckoo, get OPPRF values(A, C), voleRecv
// void voleGenMPSICS(u32 idx, u32 numParties, u32 logNum, std::vector<Socket> &chl, u32 numThreads){
//     // generate vole
//     u32 numElements = 1ull << logNum;
//     if(idx == 0){
//         std::vector<std::thread> voleGenThrds(numParties - 1);
//         for (u32 i = 1; i < numParties; ++i){
//             std::string fileName1 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS1";
//             std::string fileName2 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS2";
//             voleGenThrds[i-1] = std::thread([&, i, fileName1, fileName2]() {
//                 voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName1);
//                 voleGenRecv(idx, numElements, numThreads, chl[i-1], fileName2);
//             });            
//         }
//         for (auto& thrd : voleGenThrds) thrd.join();

//         // std::cout << "vole done " << std::endl;  
//     }
//     else{
//         std::string fileName1 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS1";
//         std::string fileName2 = "./offline/vole_" + std::to_string(numParties)+ "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS2";
//         voleGenSend(idx, numElements, numThreads, chl[0], fileName1);
//         voleGenSend(idx, numElements, numThreads, chl[0], fileName2);
//     }

//     // flush channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush()); 
//     }
//     return;
// }
