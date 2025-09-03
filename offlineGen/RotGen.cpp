#include "RotGen.h"


// for dml MPSU, if myIdx < idx, we use sender  
void rot2PartyGenMPSU(u32 isRecv, u32 numBins, Socket &chl, std::string outFileName, u32 numThreads)
{          
    PRNG prng(sysRandomSeed());
    if(isRecv)
    {    	    	
        BitVector bitV(numBins);
        AlignedVector<block> rMsgs(numBins); 

        // perform ROT as receiver, get rMsgs
        bitV.randomize(prng);       	
    	SoftRecv(numBins, bitV, chl, prng, rMsgs, numThreads);   

        // write rMsgs, bitV to outFileR 
        std::ofstream outFileR;
        outFileR.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFileR.is_open()){
            std::cout << "ROT error opening file " << outFileName << std::endl;
            return;
        }
        outFileR.write((char*)(&rMsgs[0]), numBins * sizeof(block));
        outFileR.write((char*)bitV.data(), bitV.sizeBytes());
        outFileR.close();         
    }
    else if(!isRecv)
    {
      	oc::AlignedVector<std::array<block, 2>> sMsgs(numBins);
  	      	    	    	
    	// perform ROT as sender, get sMsgs
    	SoftSend(numBins, chl, prng, sMsgs, numThreads);   	   

        // write sMsgs to outFileS
        std::ofstream outFileS;
        outFileS.open(outFileName, std::ios::binary | std::ios::out);
        if (!outFileS.is_open()){
            std::cout << "ROT error opening file " << outFileName << std::endl;
            return;
        }        
        outFileS.write((char*)(&sMsgs[0][0]), numBins * 2 * sizeof(block));
        outFileS.close(); 
    }
  
    coproto::sync_wait(chl.flush()); 

}



void rotGenMPSU(u32 idx, u32 numParties, u32 logNum, u32 numThreads)
{
    assert(idx < numParties);
    u32 numElements = 1ull << logNum;
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();

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

    for (u32 i = 0; i < numParties; ++i){
        // PORT + senderId * 100 + receiverId
        std::string outFileName = "./offline/rot_" + std::to_string(numParties) + "_" + std::to_string(numElements)+ "_P" + std::to_string(idx + 1) + std::to_string(i + 1); 
        if (i < idx){
            rot2PartyGenMPSU(1, numBins, chl[i], outFileName, numThreads);
        } else if (i > idx){
            rot2PartyGenMPSU(0, numBins, chl[i-1], outFileName, numThreads);
        }
    }

    // close channel
    for (u32 i = 0; i < chl.size(); ++i){
        coproto::sync_wait(chl[i].flush());
        coproto::sync_wait(chl[i].close());;
    }
}





