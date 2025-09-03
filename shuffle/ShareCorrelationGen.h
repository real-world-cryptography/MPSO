#pragma once

#include "../common/Defines.h"
#include "../common/permute.h"
#include <vector>


class ShareCorrelation{
public:
    ShareCorrelation(u32 numParties, u32 numElements){
        mNumParties = numParties;
        mNumElements = numElements;
    }
    ~ShareCorrelation() = default;

    // existence of share correlation
    bool exist(std::string fileName = "sc");
    // generate share correlation: pi, a, a', b, delta
    void generate();
    // write share correlation to file
    void writeToFile(std::string fileName = "sc");
    // release memory occupied by pi, a, aprime, b, delta
    void release();

    u32 mNumParties;
    u32 mNumElements;
    // permutation for party {1} to party {mNumParties}
    std::vector<std::vector<u32>> mPi;
    // a for party {2} to party {mNumParties}
    // a' for party {1} to party {mNumParties - 1}
    // b for party {1} to party {mNumParties - 1}
    std::vector<std::vector<block>> ma, maprime, mb;
    // delta for party {mNumParties}
    std::vector<block> mdelta;
};


// read pi from ShareCorrelationXor
class ShareCorrelationAdd{
public:
    ShareCorrelationAdd(u32 numParties, u32 numElements){
        mNumParties = numParties;
        mNumElements = numElements;
    }
    ~ShareCorrelationAdd() = default;

    // existence of share correlation
    bool exist(std::string fileName = "scAdd");
    // generate share correlation: pi, a, a', b, delta
    void generate(std::string fileName = "scXor");
    // write share correlation to file
    //void writePiToFile(std::string fileName = "pi");
    void writeToFile(std::string fileName = "scAdd");
    // release memory occupied by pi, a, aprime, b, delta
    void release();

    u32 mNumParties;
    u32 mNumElements;
    // permutation for party {1} to party {mNumParties}
    std::vector<std::vector<u32>> mPi;
    // a for party {2} to party {mNumParties}
    // a' for party {1} to party {mNumParties - 1}
    // b for party {1} to party {mNumParties - 1}
    std::vector<std::vector<u64>> ma, maprime, mb;
    // delta for party {mNumParties}
    std::vector<u64> mdelta;
};

class ShareCorrelationXor{
public:
    ShareCorrelationXor(u32 numParties, u32 numElements){
        mNumParties = numParties;
        mNumElements = numElements;
    }
    ~ShareCorrelationXor() = default;

    // existence of share correlation
    bool exist(std::string fileName = "scXor");
    // generate share correlation: pi, a, a', b, delta
    void generate();
    // write share correlation to file
    //void writePiToFile(std::string fileName = "pi");
    void writeToFile(std::string fileName = "scXor");
    // release memory occupied by pi, a, aprime, b, delta
    void release();

    u32 mNumParties;
    u32 mNumElements;
    // permutation for party {1} to party {mNumParties}
    std::vector<std::vector<u32>> mPi;
    // a for party {2} to party {mNumParties}
    // a' for party {1} to party {mNumParties - 1}
    // b for party {1} to party {mNumParties - 1}
    std::vector<std::vector<u64>> ma, maprime, mb;
    // delta for party {mNumParties}
    std::vector<u64> mdelta;
};





