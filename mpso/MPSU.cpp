
#include "MPSU.h"

using namespace oc;


// use the pre_generated ROT: mqssROT
std::vector<block> MPSUParty(u32 idx, u32 numParties, u32 numElements, std::vector<block> &set, u32 numThreads)
{
    oc::CuckooParam params = oc::CuckooIndex<>::selectParams(numElements, ssp, 0, 3);
    u32 numBins = params.numBins();
    
    MShuffleParty shuffleParty(idx, numParties, numBins * (numParties - 1));       
    shuffleParty.getShareCorrelation("psu");

    Timer timer;
    timer.setTimePoint("start");

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
    
    PRNG prng(sysRandomSeed());
    oc::RandomOracle hash64(8); 
    block cuckooSeed = block(0x235677879795a931, 0x784915879d3e658a);   
    std::vector<block> share((numParties - 1) * numBins, ZeroBlock);
        
    // for pmtAddOT            
    std::vector<std::thread>  pmtAddOTThrds(numParties - 1); 
    std::vector<std::vector<block>> simHashTable(numBins);
    std::vector<std::vector<block>> vvecSSROT(numParties - 1);

    // for beaver Triples Mul online
    // std::vector<std::thread>  beaTriMulThrds(numParties - idx);
    std::vector<std::vector<block>> vvecShare(numParties - 1);
 
    
    // all the parties establish simple hash table except pm
    if(idx != numParties - 1)
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
    
    if(idx == 0)
    {                              
        // mqssPMT with other parties
        for (u32 i = 1; i < numParties; ++i)
        {             
            std::string postfixPath = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1); 
            pmtAddOTThrds[i - 1] = std::thread([&, i, postfixPath]() {                
                pmtOTSend(numParties, numElements, cuckooSeed, simHashTable, chl[i - 1], vvecSSROT[i - 1], postfixPath, numThreads);
            });                               
        }
        for (auto& thrd :  pmtAddOTThrds) thrd.join();

        timer.setTimePoint("pmtAddOT done"); 
        // std::cout << "P" << idx + 1 << " 2 done" << std::endl;
        
        // beaTriMulThrds.resize(numParties-1);    	
        for (u32 j = 1; j < numParties; ++j)
        {             
            std::string postfixPath2 = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(j + 1); 
            // beaTriMulThrds[j - 1] = std::thread([&, j, postfixPath2]() {                
                beaverTripleMulOnline(idx, j+1, vvecSSROT[j-1], chl, vvecShare[j-1], postfixPath2);
            // });                               
        }
        // for (auto& thrd :  beaTriMulThrds) thrd.join();  
        timer.setTimePoint("beaverTripleMul done");
        // std::cout << "P" << idx + 1 << " 3 done" << std::endl;        
        
        // compute share by uVector
        for(u32 i = 0; i < numParties - 1; ++i)
        {
            for(u32 j = 0; j < numBins; ++j)	
            {
            	share[i*numBins + j] ^= vvecShare[i][j];            
            }                
        }
                
    	coproto::sync_wait(shuffleParty.run(chl, share));
    	timer.setTimePoint("mshuffle done");         

        // receive shares from other parties
        std::vector<std::vector<block>> recvShares(numParties - 1);
        std::vector<std::thread> recvThrds(numParties - 1);
        for (u32 i = 0; i < numParties - 1; ++i){
            recvThrds[i] = std::thread([&, i]() {
                recvShares[i].resize(share.size());
                coproto::sync_wait(chl[i].recv(recvShares[i]));
            });
        }
        for (auto& thrd : recvThrds) thrd.join();
        //timer.setTimePoint("recv share done");

        // reconstruct
        for (size_t i = 0; i < numParties - 1; i++){
            for (size_t j = 0; j < share.size(); j++){
                share[j] ^= recvShares[i][j];
            }
        }
        //timer.setTimePoint("reconstruct done");

        // check MAC
        //std::vector<block> setUnion(set);
        std::vector<block> setUnion(set);
        for (size_t i = 0; i < share.size(); i++){
            long long mac;
            hash64.Update(share[i].mData[0]);
            hash64.Final(mac);
            hash64.Reset();
            if (share[i].mData[1] == mac){
                // std::cout << "add element to union: " << share[i].mData[0] << std::endl;
                setUnion.emplace_back(share[i].mData[0]);                
            }
        }
        //timer.setTimePoint("check MAC and output done");
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
        return setUnion;                                          
    }
    else
    {   
        // establish cuckoo hash table
        oc::CuckooIndex cuckoo;
        cuckoo.init(numElements, ssp, 0, 3);
        cuckoo.insert(set, cuckooSeed);
                
        oc::RandomOracle hash64(8);        
        std::vector<block> cuckooHashTable(numBins); //x||z              
        std::vector<block> hashSet(numBins);//compute hashset: h(x)||x
        for (u32 i = 0; i < numBins; ++i)
        {
            auto bin = cuckoo.mBins[i];
            if (bin.isEmpty() == false)
            {
                auto j = bin.hashIdx();
                auto b = bin.idx();
                u64 halfK = set[b].mData[0];                
                cuckooHashTable[i] = block(halfK, j);//compute x||z
                u64 hashx;
                hash64.Update(halfK);
                hash64.Final(hashx);
                hash64.Reset();                                
                hashSet[i] = block(hashx, halfK);  //h(x)||x                                                              
            }
            else
            {          	          	
          	    hashSet[i] = prng.get();           	              
            }                                
        }
        
        // add MAC to the share
        u32 offset = (idx - 1) * numBins;
        std::copy(hashSet.begin(), hashSet.end(), share.begin() + offset);

                       
        // mqssPMT with other parties       
                
        for (u32 i = 0; i < numParties; ++i){
            std::string postfixPath = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(i + 1);                                   
            if (i < idx){
                pmtAddOTThrds[i] = std::thread([&, i, postfixPath]() {                    
                    pmtOTRecv(numParties, numElements, cuckooSeed, cuckooHashTable, chl[i], vvecSSROT[i], postfixPath, numThreads);
                });
            } else if (i > idx){
                pmtAddOTThrds[i - 1] = std::thread([&, i, postfixPath]() {               
                    pmtOTSend(numParties, numElements, cuckooSeed, simHashTable, chl[i-1], vvecSSROT[i-1], postfixPath, numThreads);                  
                });
            }
        }
        for (auto& thrd :  pmtAddOTThrds) thrd.join();        
        timer.setTimePoint("pmtAddOT done");        
        
        for (u32 j = idx; j < numParties; ++j)
        {             
            std::string postfixPath2 = std::to_string(numParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(idx + 1) + std::to_string(j + 1);
            if (j == idx){
                // beaTriMulThrds[j] = std::thread([&, j, postfixPath2]() {                
                    beaverTripleMulOnline(idx, idx+1, vvecSSROT[j-1], chl, vvecShare[j-1], postfixPath2);
                // });  
            }else if (j > idx){
                // beaTriMulThrds[j - 1] = std::thread([&, j, postfixPath2]() {                
                    beaverTripleMulOnline(idx, j+1, vvecSSROT[j-1], chl, vvecShare[j-1], postfixPath2);
                // });  
            }                                 
        }
        timer.setTimePoint("beaverTripleMul done");
        // for (auto& thrd :  beaTriMulThrds) thrd.join();        
        
    	
        // compute share by uVector
        for(u32 i = idx-1; i < numParties-1; ++i)
        {
            for(u32 j = 0; j < numBins; ++j)	
            {
            	share[i *numBins + j] ^= vvecShare[i][j];            
            }                
        }   
        coproto::sync_wait(shuffleParty.run(chl, share));
        timer.setTimePoint("mshuffle done"); 	         

        // reconstruct output
        coproto::sync_wait(chl[0].send(share));

        timer.setTimePoint("end");

        // close sockets
        for (u32 i = 0; i < chl.size(); ++i){
            coproto::sync_wait(chl[i].flush());
            coproto::sync_wait(chl[i].close());;
        }
        return std::vector<block>();
    }
}









