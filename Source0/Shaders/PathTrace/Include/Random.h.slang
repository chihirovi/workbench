#pragma once


uint hash_u32(uint x) {
    x ^= x >> 16;
    x *= 0x7feb352d;
    x ^= x >> 15;
    x *= 0x846ca68b;
    x ^= x >> 16;
    return x;
}

// Van der Corput sequence
float vanDerCorput(uint n, uint base) {
    float result = 0.0;
    float invBase = 1.0 / float(base);
    float fraction = invBase;

    while (n > 0u) {
        result += float(n % base) * fraction;
        n /= base;
        fraction *= invBase;
    }

    return result;
}

// Halton 2D (base 5, base 7)
float2 halton2D(uint index) {
    return float2(
        vanDerCorput(index, 5u),
        vanDerCorput(index, 7u)
    );
}

uint createSeed(uint frame, uint pixel_id, uint times){
    uint combined = pixel_id ^ (frame * 16777619u) ^ (times* 374761393u);
    uint seed = hash_u32(combined); 
    return seed == 0 ? 31415 : seed ;
}

struct UniformRandom {
    uint32_t times;
    uint32_t frame;
    uint32_t width;
    uint32_t2 pixel_id;
    
    __init(uint32_t f, uint32_t w, uint32_t2 p){
        times = 0;
        frame = f;
        width = w;
        pixel_id = p;
    }
    
    [mutating]
    float2 rand(){
        float2 temp = halton2D(createSeed(frame, width*pixel_id.y+pixel_id.x, times));
        times += 1;
        return temp;
    }
};


