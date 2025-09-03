#include "permute.h"



void genPermutation(u32 size, std::vector<u32> &pi)
{
    pi.resize(size);
    for (size_t i = 0; i < pi.size(); ++i){
        pi[i] = i;
    }
    std::shuffle(pi.begin(), pi.end(), global_built_in_prg2);
    return;
}

void permute64(std::vector<u32> &pi, std::vector<u64> &data){
    std::vector<u64> res(data.size());
    for (size_t i = 0; i < pi.size(); ++i){
        res[i] = data[pi[i]];
    }
    data.assign(res.begin(), res.end());
}

void permute(std::vector<u32> &pi, std::vector<block> &data){
    std::vector<block> res(data.size());
    for (size_t i = 0; i < pi.size(); ++i){
        res[i] = data[pi[i]];
    }
    data.assign(res.begin(), res.end());
}
