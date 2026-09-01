#pragma once
#include "Irradiance.h.slang"
#include "RayDirection.h.slang"

RayDesc create_probe_ray(DDGIDesc desc, uint32_t id_1d, uint32_t ray_idx, RWStructuredBuffer<float4> offsets) {
    float3 origin = probe_pos_from_index1D(desc, id_1d, offsets);
    float3 direction = get_probe_ray_direction(desc, ray_idx);

    RayDesc ray_desc;
    ray_desc.Origin = origin;
    ray_desc.Direction = direction;
    ray_desc.TMin = 0;
    ray_desc.TMax = 1e5;
    
    return ray_desc;
}

void recode_probe_trace_result(RWStructuredBuffer<float4> dst_result, uint32_t id_1d, uint32_t ray_idx, float3 radiance, float hit_t) {
    //dst_result[id_1d * DDGI_PROBE_NUM_RAY + ray_idx] = float4(float16_t(radiance.r), float16_t(radiance.g), float16_t(radiance.g), float16_t(hit_t));
    dst_result[id_1d * DDGI_PROBE_NUM_RAY + ray_idx] = float4(radiance, hit_t);
}

