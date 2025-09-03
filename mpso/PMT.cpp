#include "PMT.h"

// for MPSU, if x doesn't exist in Y, get same string
// get the OPPRF key
void pmtOTSend(u32 numParties, u32 numElements, block cuckooSeed, std::vector<std::vector<block>> &simHashTable, Socket& chl, std::vector<block> &ssrotOut, std::string postfixPath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = simHashTable.size();  

    // open all the needed files
    if(postfixPath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }
    // postfixPath = std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(numParties) ; 
    std::string volePath = "./offline/vole_" + postfixPath; 
    std::string triplePath = "./offline/triple_" + postfixPath;
    std::string rotPath = "./offline/rot_" + postfixPath;

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    std::ifstream tripleFile;
    tripleFile.open(triplePath, std::ios::binary | std::ios::in);
    if (!tripleFile.is_open()){
        std::cout << "Error opening file " << triplePath << std::endl;
        return;
    }

    std::ifstream rotFile;
    rotFile.open(rotPath, std::ios::binary | std::ios::in);
    if (!rotFile.is_open()){
        std::cout << "Error opening file " << rotPath << std::endl;
        return;
    }

    // read vole from file
    block mD;
    std::vector<block> mB(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)(&mD), sizeof(block));
        voleFile.read((char*)mB.data(), mB.size() * sizeof(block));        
    }
    voleFile.close();

    // diffC received
    std::vector<block> diffC(numBins);
    coproto::sync_wait(chl.recv(diffC));

    // send paxosSeed
    block paxosSeed = prng.get();    
    coproto::sync_wait(chl.send(paxosSeed));


    // set for PRF(k, x||z)    
    oc::AES hasher;
    hasher.setKey(cuckooSeed);

    u32 interactNum = (numParties * (numParties-1))/2;
    u32 keyBitLength = ssp + oc::log2ceil(numBins * interactNum);  
    u32 keyByteLength = oc::divCeil(keyBitLength, 8);

    // get random labels
    mMatrix<u8> mLabel(numBins, keyByteLength);
    prng.get(mLabel.data(), mLabel.size());
    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    std::vector<block> vecK(numElements * 3);
    mMatrix<u8> values(numElements * 3, keyByteLength);

    u32 countV = 0;
    for (u32 i = 0; i < numBins; ++i)
    {
    	auto size = simHashTable[i].size();
    	for(u32 p = 0; p < size; ++p)
    	{
    	    auto hyj = simHashTable[i][p];
    	    vecK[countV] = hyj;
    	    hyj ^= diffC[i];
    	    auto tmp2 = mB[i] ^ (hyj.gf128Mul(mD));
    	    tmp2 = hasher.hashBlock(tmp2);
    	    memcpy(&values(countV, 0), &tmp2, keyByteLength);
    	    for(u64 ii = 0; ii < keyByteLength; ++ii)
    	    {
    	    	values(countV, ii) = values(countV, ii) ^ mLabel(i,ii); 
    	    }
    	    countV += 1;    	    	
    	}        
    }
    
    mMatrix<u8> okvs(mPaxos.size(), keyByteLength);
    mPaxos.setInput(vecK);
    mPaxos.encode<u8>(values, okvs);    
    // when using multi threads for okvs, there might be some problems    
    coproto::sync_wait(chl.send(okvs));
    
    // call gmw
    volePSI::BetaCircuit cir = volePSI::isZeroCircuit(keyBitLength);
    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, 1, prng.get());
    std::vector<block> a, b, c, d;
    u64 numTriples = cmp.mNumOts / 2;
    
    a.resize(numTriples / 128, ZeroBlock);
    b.resize(numTriples / 128, ZeroBlock);
    c.resize(numTriples / 128, ZeroBlock);
    d.resize(numTriples / 128, ZeroBlock);
    if (tripleFile.is_open()){
        //tripleFile.seekg(0L, std::ios::beg);
        tripleFile.read((char*)a.data(), a.size() * sizeof(block));
        tripleFile.read((char*)b.data(), b.size() * sizeof(block));
        tripleFile.read((char*)c.data(), c.size() * sizeof(block));
        tripleFile.read((char*)d.data(), d.size() * sizeof(block));
    }    
    tripleFile.close();
    cmp.setTriples(a, b, c, d);    

    cmp.setInput(0, mLabel);
    coproto::sync_wait(cmp.run(chl));
    
    mMatrix<u8> mOut;
    mOut.resize(numBins, 1);
    cmp.getOutput(0, mOut);
    
    // get the bitvector
    BitVector out(numBins);
    for (u32 i = 0; i < numBins; ++i){
        out[i] = mOut(i, 0) & 1;
    }   

    // ssROT sender
    BitVector bitDelta(numBins);
    AlignedVector<std::array<block, 2>> sMsgs(numBins);

    rotFile.read((char*)sMsgs.data(), 2 * numBins * sizeof(block));
    rotFile.close();
    coproto::sync_wait(chl.recv(bitDelta));

    out ^= bitDelta;
    ssrotOut.resize(numBins);
    for (u32 i = 0; i < numBins; ++i){
        ssrotOut[i] = sMsgs[i][out[i]];
    }


   return;    
}


// get the OPPRF values
void pmtOTRecv(u32 numParties, u32 numElements, block cuckooSeed, std::vector<block> &cuckooHashTable, Socket &chl, std::vector<block> &ssrotOut, std::string postfixPath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = cuckooHashTable.size();

    // open all the needed files
    if(postfixPath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }
    // postfixPath = std::to_string(totalParties) + "_" + std::to_string(numElements) + "_P" + std::to_string(i + 1) + std::to_string(numParties) ; 
    std::string volePath = "./offline/vole_" + postfixPath; 
    std::string triplePath = "./offline/triple_" + postfixPath;
    std::string rotPath = "./offline/rot_" + postfixPath;

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    std::ifstream tripleFile;
    tripleFile.open(triplePath, std::ios::binary | std::ios::in);
    if (!tripleFile.is_open()){
        std::cout << "Error opening file " << triplePath << std::endl;
        return;
    }

    std::ifstream rotFile;
    rotFile.open(rotPath, std::ios::binary | std::ios::in);
    if (!rotFile.is_open()){
        std::cout << "Error opening file " << rotPath << std::endl;
        return;
    }

    // send diffC
    std::vector<block> mA(numBins, ZeroBlock);
    std::vector<block> mC(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)mA.data(), mA.size() * sizeof(block));
        voleFile.read((char*)mC.data(), mC.size() * sizeof(block));
    }
    std::vector<block> diffC(numBins); 
    for (u32 i = 0; i < numBins; ++i)
    {
    	diffC[i] = cuckooHashTable[i] ^ mC[i];                                            
    }     
    coproto::sync_wait(chl.send(diffC));

    // recv paxosSeed
    block paxosSeed;
    coproto::sync_wait(chl.recv(paxosSeed));

    oc::AES hasher;
    hasher.setKey(cuckooSeed);    

    u32 interactNum = (numParties * (numParties-1))/2;
    u32 keyBitLength = ssp + oc::log2ceil(numBins * interactNum);  
    u32 keyByteLength = oc::divCeil(keyBitLength, 8);


    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    mMatrix<u8> values(numBins, keyByteLength);
    mMatrix<u8> okvs(mPaxos.size(), keyByteLength);
    coproto::sync_wait(chl.recv(okvs));
    mPaxos.decode<u8>(cuckooHashTable, values, okvs);
    
    // compute mLabel = values ^ hash(mA)
    mMatrix<u8> mLabel(numBins, keyByteLength);
    
    for (u32 i = 0; i < numBins; ++i)
    {
    	auto prf = hasher.hashBlock(mA[i]);
    	memcpy(&mLabel(i,0), &prf, keyByteLength);
    	for(u32 ii = 0; ii < keyByteLength; ++ii)
    		mLabel(i, ii) ^= values(i,ii);          
    }
        
    volePSI::BetaCircuit cir = volePSI::isZeroCircuit(keyBitLength);
    volePSI::Gmw cmp;
    cmp.init(mLabel.rows(), cir, numThreads, 0, prng.get());
    cmp.implSetInput(0, mLabel, mLabel.cols());;
    
    std::vector<block> a, b, c, d;
    u64 numTriples = cmp.mNumOts / 2;
    a.resize(numTriples / 128, ZeroBlock);
    b.resize(numTriples / 128, ZeroBlock);
    c.resize(numTriples / 128, ZeroBlock);
    d.resize(numTriples / 128, ZeroBlock);

    if (tripleFile.is_open()){
        //tripleFile.seekg(0L, std::ios::beg);
        tripleFile.read((char*)a.data(), a.size() * sizeof(block));
        tripleFile.read((char*)b.data(), b.size() * sizeof(block));
        tripleFile.read((char*)c.data(), c.size() * sizeof(block));
        tripleFile.read((char*)d.data(), d.size() * sizeof(block));
    }
    tripleFile.close();
    cmp.setTriples(a, b, c, d);    
  
    coproto::sync_wait(cmp.run(chl));
    
    mMatrix<u8> mOut;
    mOut.resize(numBins, 1);
    cmp.getOutput(0, mOut);
    
    BitVector out(numBins); 
    for (u32 i = 0; i < numBins; ++i){
        out[i] = mOut(i, 0) & 1;
    }        

    // ssROT receiver
    BitVector bitDelta(numBins);
    ssrotOut.resize(numBins);

    rotFile.read((char*)ssrotOut.data(), numBins * sizeof(block));
    rotFile.read((char*)bitDelta.data(), bitDelta.sizeBytes());
    rotFile.close();

    // compute the difference of bitDelta and bitV
    bitDelta ^= out;
    coproto::sync_wait(chl.send(bitDelta));  
    
    return;  

}







// for MPSI/MSPIC and part of MPSICS, if x exist in Y, get same strings

// get the OPPRF key
void opprfSendPSI64(u32 numParties, u32 numElements, block cuckooSeed, std::vector<std::vector<block>> &simHashTable, Socket& chl, std::vector<u64> &opprfOut, std::string volePath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = simHashTable.size();  

    // open all the needed files
    if(volePath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    // read vole from file
    block mD;
    std::vector<block> mB(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)(&mD), sizeof(block));
        voleFile.read((char*)mB.data(), mB.size() * sizeof(block));        
    }
    voleFile.close();

    // diffC received
    std::vector<block> diffC(numBins);
    coproto::sync_wait(chl.recv(diffC));

    // send paxosSeed
    block paxosSeed = prng.get();    
    coproto::sync_wait(chl.send(paxosSeed));


    // get random labels
    std::vector<u64> mLabel(numBins);
    prng.get(mLabel.data(), mLabel.size());
    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    std::vector<block> vecK(numElements * 3);
    mMatrix<u8> values(numElements * 3, 8);
    mMatrix<u8> okvs(mPaxos.size(), 8);

    u32 countV = 0;
    oc::RandomOracle hash64(8);
    for (u32 i = 0; i < numBins; ++i)
    {
    	auto size = simHashTable[i].size();
    	for(u32 p = 0; p < size; ++p)
    	{
    	    auto hyj = simHashTable[i][p];
    	    vecK[countV] = hyj;
    	    hyj ^= diffC[i];
    	    auto tmp2 = mB[i] ^ (hyj.gf128Mul(mD));
            u64 hashTo64;
            hash64.Update(tmp2);
            hash64.Final(hashTo64);
            hash64.Reset();

            hashTo64 ^= mLabel[i];// values[i] = hashTo64 ^ mLabel
            memcpy(&values(countV, 0), &hashTo64, 8);
    	    countV += 1;    	    	
    	}        
    }
       
    mPaxos.setInput(vecK);
    mPaxos.encode<u8>(values, okvs);   
    // when using multi threads for okvs, there might be some problems    
    coproto::sync_wait(chl.send(okvs));
    
    opprfOut.resize(numBins);
    
    for (u32 i = 0; i < numBins; ++i){
        opprfOut[i] = mLabel[i];      
    }

   return;    
}

//get the OPPRF values
void opprfRecvPSI64(u32 numParties, u32 numElements, block cuckooSeed, std::vector<block> &cuckooHashTable, Socket &chl, std::vector<u64> &opprfOut, std::string volePath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = cuckooHashTable.size();

    // open all the needed files
    if(volePath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    // send diffC
    std::vector<block> mA(numBins, ZeroBlock);
    std::vector<block> mC(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)mA.data(), mA.size() * sizeof(block));
        voleFile.read((char*)mC.data(), mC.size() * sizeof(block));
    }
    std::vector<block> diffC(numBins); 
    for (u32 i = 0; i < numBins; ++i)
    {
    	diffC[i] = cuckooHashTable[i] ^ mC[i];                                            
    }     
    coproto::sync_wait(chl.send(diffC));

    // recv paxosSeed
    block paxosSeed;
    coproto::sync_wait(chl.recv(paxosSeed));  

    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    mMatrix<u8> values(numBins, 8);
    mMatrix<u8> okvs(mPaxos.size(), 8);
    coproto::sync_wait(chl.recv(okvs));
    mPaxos.decode<u8>(cuckooHashTable, values, okvs);
    
    // compute mLabel = values ^ hash(mA)
    oc::RandomOracle hash64(8);       
    opprfOut.resize(numBins);
    for (u32 i = 0; i < numBins; ++i)
    {
        u64 hashTo64;
        hash64.Update(mA[i]);
        hash64.Final(hashTo64);
        hash64.Reset();        
        memcpy(&opprfOut[i], &values(i,0), 8);
        opprfOut[i] ^= hashTo64; // opprfOut[i] = oprfValue ^ hashTo64R = mLabel[i] ^ hashTo64S ^ hashTo64R   
    }
   
    return;  
}




// for part of MPSICS, get OPPRF values of u64

// get the OPPRF key
void opprfSendPSICS64(u32 numParties, u32 numElements, block cuckooSeed, std::vector<std::vector<block>> &simHashTable, Socket& chl, std::vector<u64> &opprfOut, std::string volePath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = simHashTable.size();  

    // open all the needed files
    if(volePath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    // read vole from file
    block mD;
    std::vector<block> mB(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)(&mD), sizeof(block));
        voleFile.read((char*)mB.data(), mB.size() * sizeof(block));        
    }
    voleFile.close();

    // diffC received
    std::vector<block> diffC(numBins);
    coproto::sync_wait(chl.recv(diffC));

    // send paxosSeed
    block paxosSeed = prng.get();    
    coproto::sync_wait(chl.send(paxosSeed));

    std::vector<u64> mLabel(numBins);
    prng.get(mLabel.data(), mLabel.size());

    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    std::vector<block> vecK(numElements * 3);
    mMatrix<u8> values(numElements * 3, 8);
    mMatrix<u8> okvs(mPaxos.size(), 8);

    oc::RandomOracle hash64(8); 
    u32 countV = 0;
    for (u32 i = 0; i < numBins; ++i)
    {
    	auto size = simHashTable[i].size();
    	for(u32 p = 0; p < size; ++p)
    	{
    	    auto hyj = simHashTable[i][p];
            u64 w = hyj.mData[1] - mLabel[i];// w = x-s

    	    vecK[countV] = hyj;
    	    hyj ^= diffC[i];
    	    auto tmp = mB[i] ^ (hyj.gf128Mul(mD));

            u64 hashTo64;
            hash64.Update(tmp);
            hash64.Final(hashTo64);
            hash64.Reset();   

            w ^= hashTo64; // w = (x-s)^oppfValue
            memcpy(&values(countV,0), &w, 8);
    	    countV += 1;    	    	
    	}        
    }
    
    mPaxos.setInput(vecK);
    mPaxos.encode<u8>(values, okvs);    
    // when using multi threads for okvs, there might be some problems    
    coproto::sync_wait(chl.send(okvs));    

    opprfOut.resize(numBins);
    for (u32 i = 0; i < numBins; ++i){
        opprfOut[i] = mLabel[i];
    }

   return;    
}


// get OPPRF values
void opprfRecvPSICS64(u32 numParties, u32 numElements, block cuckooSeed, std::vector<block> &cuckooHashTable, Socket &chl, std::vector<u64> &opprfOut, std::string volePath, u32 numThreads)
{
    PRNG prng(sysRandomSeed());
    u32 numBins = cuckooHashTable.size();

    // open all the needed files
    if(volePath.empty()){
        std::cout << "Empty pre-files! " << std::endl;
        return;
    }

    std::ifstream voleFile;
    voleFile.open(volePath, std::ios::binary | std::ios::in);
    if (!voleFile.is_open()){
        std::cout << "Error opening file " << volePath << std::endl;
        return;
    }

    // send diffC
    std::vector<block> mA(numBins, ZeroBlock);
    std::vector<block> mC(numBins, ZeroBlock);
    if (voleFile.is_open()){
        voleFile.read((char*)mA.data(), mA.size() * sizeof(block));
        voleFile.read((char*)mC.data(), mC.size() * sizeof(block));
    }
    std::vector<block> diffC(numBins); 
    for (u32 i = 0; i < numBins; ++i)
    {
    	diffC[i] = cuckooHashTable[i] ^ mC[i];                                            
    }     
    coproto::sync_wait(chl.send(diffC));

    // recv paxosSeed
    block paxosSeed;
    coproto::sync_wait(chl.recv(paxosSeed));

    Paxos<u32> mPaxos;
    mPaxos.init(numElements * 3, 3, ssp, PaxosParam::Binary, paxosSeed);
    mMatrix<u8> values(numBins, 8);
    mMatrix<u8> okvs(mPaxos.size(), 8);
    coproto::sync_wait(chl.recv(okvs));
    mPaxos.decode<u8>(cuckooHashTable, values, okvs);

    oc::RandomOracle hash64(8); 
    opprfOut.resize(numBins);
    for (u32 i = 0; i < numBins; ++i)
    {
        u64 hashTo64;
        hash64.Update(mA[i]);
        hash64.Final(hashTo64);
        hash64.Reset();          
        u64 value64;
        memcpy(&value64, &values(i,0), 8); 

        opprfOut[i] = value64 ^ hashTo64;// to get x-s
    }
    return;  
}



