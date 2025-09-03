
#include "MPSICS.h"



// P_0 cuckoo, others simple hash
u64 MPSICardSumParty(u32 idx, u32 numParties, u32 numElements, std::vector<block> &set, u32 numThreads)
{
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();

    // for oprf values (as indicator)
    MShuffleParty64 shuffleParty1(idx, numParties, numBins);       
    shuffleParty1.getShareCorrelation("psics1");    

    // use for item sum
    MShuffleParty64 shuffleParty2(idx, numParties, numBins);       
    shuffleParty2.getShareCorrelation("psics2");   

    Timer timer;
    timer.setTimePoint("start");

    std::vector<Socket> chl;
    for (u32 i = 0; i < numParties; ++i){
        // PORT + senderId * 100 + receiverId
        if (i < idx){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + idx * 100 + i), true));
        } else if (i > idx){
            chl.emplace_back(coproto::asioConnect("localhost:" + std::to_string(PORT + i * 100 + idx), false));
        }
    }

    PRNG prng(sysRandomSeed());
    block cuckooSeed = block(0x235677879795a931, 0x784915879d3e658a);      

    // for computing cuckoo set sum
    // std::vector<u64> set64(numBins, 0);
    BitVector BitIndi(numBins);

    // for opprf            
    std::vector<std::thread>  opprfThrds(numParties - 1); 
    std::vector<std::vector<block>> simHashTable(numBins);
    std::vector<block> cuckooHashTable(numBins); //x||0, for P_0
    std::vector<std::vector<u64>> vvecOPPRF(numParties - 1);// use as indicator
    std::vector<std::vector<u64>> vvecOPPRF64(numParties - 1); // for item sum
    std::vector<u64> vecOPPRF(numBins);// before btMul
    std::vector<u64> itemSum(numBins);// before btMul
    std::vector<u64> vecBTMul(numBins);

    // all the parties establish simple hash table except P_0
    if(idx != 0)
    {
    	volePSI::SimpleIndex sIdx;
    	sIdx.init(numBins, numElements, ssp, 3);
    	sIdx.insertItems(set, cuckooSeed);
    	for (u64 i = 0; i < numBins; ++i)
    	{
    	    auto bin = sIdx.mBins[i];
    	    auto size = sIdx.mBinSizes[i];
    	    simHashTable[i].resize(size);
    	    for (u64 p = 0; p < size; ++p)
    	    {
    	    	auto j = bin[p].hashIdx();
    	    	auto b = bin[p].idx();
    	    	u64 halfK = set[b].mData[0];
    	    	simHashTable[i][p] = block(halfK, j);    	    	   	    
    	    }    	        	        	        	
    	}   
    }

    if(idx == 0){
        // all the party
        oc::CuckooIndex cuckoo;
        cuckoo.init(numElements, ssp, 0, 3);
        cuckoo.insert(set, cuckooSeed);
                                
        for (u32 i = 0; i < numBins; ++i)
        {
            auto bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                u64 halfK = set[b].mData[0];                
                cuckooHashTable[i] = block(halfK, j);//compute x||z                               
                // set64[i] = halfK;  //x||0                                                             
            }                              
        }       

        for (u32 i = 1; i < numParties; ++i)
        {             
            std::string opprfPath1 = "./offline/vole_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS1"; 
            std::string opprfPath2 = "./offline/vole_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1) + "PSICS2"; 
            opprfThrds[i - 1] = std::thread([&, i, opprfPath1, opprfPath2]() {                
                opprfRecvPSI64(numParties, numElements, cuckooSeed, cuckooHashTable, chl[i - 1], vvecOPPRF[i - 1], opprfPath1, numThreads);
                opprfRecvPSICS64(numParties, numElements, cuckooSeed, cuckooHashTable, chl[i - 1], vvecOPPRF64[i - 1], opprfPath2, numThreads);
            });                               
        }
        for (auto& thrd :  opprfThrds) thrd.join();

        timer.setTimePoint("opprf done"); 

        
        // add all the OPPRF and then ssROT values
        for (u32 i = 0; i < numBins; ++i){
            for (u32 j = 0; j < numParties-1; ++j){
                vecOPPRF[i] ^= vvecOPPRF[j][i];
                itemSum[i] += vvecOPPRF64[j][i];
            }
        } 


        std::string postfixPath2 = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1)  + std::to_string(numParties) + "PSI64"; 
        beaverTripleMulOnline64(idx, numParties, vecOPPRF, chl, vecBTMul, postfixPath2);

        timer.setTimePoint("beaverTripleMul done");
     
        // compute share = vecBTMul
        std::vector<u64> shareSum(vecBTMul);     

        coproto::sync_wait(shuffleParty1.runXOR(chl, shareSum));    
        coproto::sync_wait(shuffleParty2.runADD(chl, itemSum));
        
    	timer.setTimePoint("mshuffle done");         

        // receive shares from other parties
        std::vector<std::vector<u64>> recvShares(numParties - 1);
        std::vector<std::thread> recvThrds(numParties - 1);
        for (u32 i = 0; i < numParties - 1; ++i){
            recvThrds[i] = std::thread([&, i]() {
                recvShares[i].resize(numBins);
                coproto::sync_wait(chl[i].recv(recvShares[i]));
            });
        }
        for (auto& thrd : recvThrds) thrd.join();
        //timer.setTimePoint("recv share done");

        // reconstruct
        for (size_t i = 0; i < numParties - 1; i++){
            for (size_t j = 0; j < numBins; j++){
                shareSum[j] ^= recvShares[i][j];
            }
        }

        //timer.setTimePoint("reconstruct done");

        // check indicator
        u64 insectionSum = 0;
        for (size_t i = 0; i < numBins; i++){
            if (shareSum[i] == 0){    
                BitIndi[i] = 1;
                insectionSum += itemSum[i];      
            }
        }


        for (u32 i = 0; i < numParties - 1; ++i){
            coproto::sync_wait(chl[i].send(BitIndi));
        }


        std::vector<u64> recvSum(numParties - 1);
        for (size_t i = 0; i < numParties - 1; i++){
            coproto::sync_wait(chl[i].recv(recvSum[i]));
        }
    
        for (size_t i = 0; i < numParties - 1; i++){
            insectionSum += recvSum[i];
        }

        timer.setTimePoint("end");


        // close sockets
        double comm = 0;
        for (u32 i = 0; i < chl.size(); ++i){
            comm += chl[i].bytesReceived() + chl[i].bytesSent();
            coproto::sync_wait(chl[i].flush());
            coproto::sync_wait(chl[i].close());;
        }
        
        std::cout << "P1 communication cost = " << std::fixed << std::setprecision(3) << comm / 1024 / 1024 << " MB" << std::endl;   
        std::cout << timer << std::endl;
        return insectionSum;  
    }
    else{

        std::string opprfPath1 = "./offline/vole_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS1"; 
        std::string opprfPath2 = "./offline/vole_" + std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(1) + "PSICS2"; 
        opprfSendPSI64(numParties, numElements, cuckooSeed, simHashTable, chl[0], vecOPPRF, opprfPath1, numThreads);
        opprfSendPSICS64(numParties, numElements, cuckooSeed, simHashTable, chl[0], itemSum, opprfPath2, numThreads);

        std::string postfixPath2 = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(numParties) + "PSI64"; 
        beaverTripleMulOnline64(idx, numParties, vecOPPRF, chl, vecBTMul, postfixPath2);


        // shuffle block to get indicator string, shuffle set64 by the same permutation 
        coproto::sync_wait(shuffleParty1.runXOR(chl, vecBTMul));
        coproto::sync_wait(shuffleParty2.runADD(chl, itemSum));
        

        coproto::sync_wait(chl[0].send(vecBTMul));

        // recv BitIndi from P0
        coproto::sync_wait(chl[0].recv(BitIndi));

        u64 sumI = 0;
        // compute set sum, send to P0
        u32 cout = 0;
        for (size_t i = 0; i < numBins; i++){
            if(BitIndi[i]){
                sumI += itemSum[i];
            }            
        }

        coproto::sync_wait(chl[0].send(sumI));

        // close sockets
        for (u32 i = 0; i < chl.size(); ++i){
            coproto::sync_wait(chl[i].flush());
            coproto::sync_wait(chl[i].close());;
        }
        return 0;

    }

}







