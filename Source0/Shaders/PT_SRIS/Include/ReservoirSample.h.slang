#pragma once

#include "LightSample.h.slang"

struct ReservoirSample {
    LightSample light_sample; //40
    float3 first_hit_position; //52
    float16_t3 first_hit_normal; //58
    uint8_t3 first_hit_albedo; //61
    uint8_t3 first_hit_specular;  //64
    uint8_t first_hit_roughness; //65
    uint8_t first_hit_metalness; //66
    uint8_t _padding0; //67
    uint8_t _padding1; //68
};