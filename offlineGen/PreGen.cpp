#include "PreGen.h"
//set up for offline stage


// only need VOLE, and one round [beaver triple of u64]
void MPSIpreGen(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
{
    voleGenMPSI(idx, numParties, logNum, numThreads);
    if(idx == 0){
        beaverGenMPSI64(numParties, logNum);
        std::cout << "pre done " << std::endl;
    }
    

}

// need VOLE, one round [beaver triple of u64], and one round sc64
void MPSICpreGen(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
{
    voleGenMPSI(idx, numParties, logNum, numThreads);
    if(idx == 0){
        beaverGenMPSI64(numParties, logNum);
        std::cout << "pre done " << std::endl;

        oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
        u32 numBins = params.numBins();        
        ShareCorrelationXor sc(numParties, numBins);
        sc.generate();
        sc.writeToFile("psic");
        sc.release();
        std::cout << "generate sc64 done" << std::endl;           
    }

}

// need VOLE, one round [beaver triple of u64], and two round sc64
void MPSICSpreGen(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
{
    voleGenMPSICS(idx, numParties, logNum, numThreads);
    if(idx == 0){
        beaverGenMPSI64(numParties, logNum);
        std::cout << "pre done " << std::endl;

        oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
        u32 numBins = params.numBins();    

        ShareCorrelationXor sc1(numParties, numBins);
        sc1.generate();
        sc1.writeToFile("psics1");
        sc1.release();

        ShareCorrelationAdd sc2(numParties, numBins);
        sc2.generate("psics1");
        sc2.writeToFile("psics2");
        sc2.release();        
        std::cout << "generate sc64 done" << std::endl;           
    }
}


// need VOLE, one round [beaver triple of u64], and one round sc64
void MPSUpreGen(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
{
    voleGenMPSU(idx, numParties, logNum, numThreads);
    rotGenMPSU(idx, numParties, logNum, numThreads);
    boolTriplesGenMPSU(idx, numParties, logNum, numThreads);
    if(idx == 0){
        oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
        u32 numBins = params.numBins();    

        ShareCorrelation sc(numParties, (numParties - 1) * numBins);
        sc.generate();
        sc.writeToFile("psu");
        sc.release();
        std::cout << "generate sc done" << std::endl;
        beaverGenMPSU(numParties, logNum);
        std::cout << "generate beaver done" << std::endl;     
    }

}




// // only need VOLE, and one round [beaver triple of u64]
// void preGenMPSI64(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
// {
//     // connect
//     std::vector<Socket> chl;
//     if(idx == 0){
//         for (u32 i = 1; i < numParties; ++i){
//             chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
//         }
//     } else{
//         chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
//     }

//     voleGenMPSI(idx, numParties, logNum, chl, numThreads);

//     // close channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush());
//         chl[i].close();
//     }

//     if(idx == 0){
//         beaverGenMPSI64(numParties, logNum);
//         std::cout << "Offline preGen done" << std::endl; 
//     }
// }

// // need VOLE, one round [beaver triple of u64], and one round sc64
// void preGenMPSIC64(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
// {
//     // connect
//     std::vector<Socket> chl;
//     if(idx == 0){
//         for (u32 i = 1; i < numParties; ++i){
//             chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
//         }
//     } else{
//         chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
//     }

//     voleGenMPSI(idx, numParties, logNum, chl, numThreads);

//     // close channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush());
//         chl[i].close();
//     }    
    
//     if(idx == 0){
//         beaverGenMPSI64(numParties, logNum);

//         oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
//         u32 numBins = params.numBins();        
//         ShareCorrelationXor sc(numParties, numBins);
//         sc.generate();
//         sc.writeToFile("psic");
//         sc.release();

//         std::cout << "Offline preGen done" << std::endl;          
//     }
// }

// // need VOLE, one round [beaver triple of u64], and two round sc64
// void preGenMPSICS64(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
// {
//     // connect
//     std::vector<Socket> chl;
//     if(idx == 0){
//         for (u32 i = 1; i < numParties; ++i){
//             chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
//         }
//     } else{
//         chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
//     }

//     voleGenMPSI(idx, numParties, logNum, chl, numThreads);

//     // close channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush());
//         chl[i].close();
//     }

//     if(idx == 0){
//         beaverGenMPSI64(numParties, logNum);

//         oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
//         u32 numBins = params.numBins();    

//         ShareCorrelationXor sc1(numParties, numBins);
//         sc1.generate();
//         sc1.writeToFile("psics1");
//         sc1.release();

//         ShareCorrelationAdd sc2(numParties, numBins);
//         sc2.generate("psics1");
//         sc2.writeToFile("psics2");
//         sc2.release();        
//         std::cout << "Offline preGen done" << std::endl;           
//     }
// }


// // need VOLE, one round [beaver triple of u64], and one round sc64
// void preGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads, bool fakeBase)
// {
//     // connect
//     std::vector<Socket> chl;
//     if(idx == 0){
//         for (u32 i = 1; i < numParties; ++i){
//             chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
//         }
//     } else{
//         chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + 0), true));
//     }

//     voleGenMPSU(idx, numParties, logNum, chl, numThreads);
//     rotGenMPSU(idx, numParties, logNum, chl, numThreads, fakeBase);
//     boolTriplesGenMPSU(idx, numParties, logNum, chl, numThreads);

//     // close channel
//     for (u32 i = 0; i < chl.size(); ++i){
//         coproto::sync_wait(chl[i].flush());
//         chl[i].close();
//     }    
    
//     if(idx == 0){
//         beaverGenMPSU(numParties, logNum);

//         oc::CuckooParam params = oc::CuckooIndex<>::selectParams(1 << logNum, ssp, 0, 3);
//         u32 numBins = params.numBins();    

//         ShareCorrelation sc(numParties, (numParties - 1) * numBins);
//         sc.generate();
//         sc.writeToFile("psu");
//         sc.release();

//         std::cout << "Offline preGen done" << std::endl;    
//     }
//     return;

// }

