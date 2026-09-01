#pragma once

struct Random{
    uint4 seed;
    
    __init(uint3 s){
        seed = uint4(s, 0);
    }
    
    [mutating]
    float Get(){
        const uint4 mult = uint4(0xae3cc725, 0x9fe72885, 0xae36bfb5, 0x82c1fcad);

        uint4 u = uint4(seed);
        u = u * mult;
        u ^= u.wxyz ^ u >> 13;
        
        uint r = dot(u, mult);

        r ^= r >> 11;
        r = (r * r) ^ r;
        
        // update seed
        seed[1] += 1u;
        seed[3] += 1u;
            
        return r * 2.3283064365386962890625e-10;
    }
};