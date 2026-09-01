#pragma once

uint16_t encode_spherical_16(float3 nor){
    float2 v = float2(0.5 + 0.5 * atan2(nor.z, nor.x) / 3.141593, acos(nor.y) / 3.141593);
    uint2 d = uint2(round(v * 255.0));
    return uint16_t(d.x | (d.y << 8u));
}

float3 decode_spherical_16(uint16_t data){
    uint32_t d = data;
    float2 v = float2(d & 255u, d >> 8) / 255.0;
    v.x = 2.0 * v.x - 1.0;
    v *= 3.141593;
    return normalize(float3(sin(v.y) * cos(v.x), cos(v.y), sin(v.y) * sin(v.x)));
}