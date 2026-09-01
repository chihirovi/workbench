#pragma once
#include "Types.h.slang"

uint32_t3 probe_index_1D_to_3D(DDGIDesc desc, uint id_1d){
    uint32_t3 sizes = desc.sizes.xyz;
    return uint32_t3(
        id_1d%sizes.x,
        (id_1d%(sizes.x*sizes.y))/sizes.x,
        id_1d/(sizes.x*sizes.y)
    );
}

uint probe_index_3D_to_1D(DDGIDesc desc, uint32_t3 id_3d){
    uint32_t3 sizes = desc.sizes.xyz;
    return id_3d.x + id_3d.y*sizes.x + id_3d.z*(sizes.x*sizes.y);
}

float3 probe_pos_from_index3D(DDGIDesc desc, uint32_t3 id_3d, RWStructuredBuffer<float4> offsets){
    float3 base   = desc.bottom_left_front_origin.xyz;
    float3 scales = desc.scales.xyz;
    return base + scales * id_3d + offsets[probe_index_3D_to_1D(desc, id_3d)].xyz; ;
}

float3 probe_pos_from_index1D(DDGIDesc desc, uint id_1d, RWStructuredBuffer<float4> offsets){
    return probe_pos_from_index3D(desc, probe_index_1D_to_3D(desc, id_1d), offsets);
}

uint32_t3 get_base_probe_grid_coords(DDGIDesc desc, float3 world_pos){
    float3 base   = desc.bottom_left_front_origin.xyz;
    float3 scales = desc.scales.xyz;
    float3 tmp = world_pos - base;
    uint32_t3 coord = uint32_t3(
        uint(tmp.x/scales.x),
        uint(tmp.y/scales.y),
        uint(tmp.z/scales.z)
    );
    
    uint32_t3 sizes = desc.sizes.xyz;
    return clamp(coord, uint32_t3(0,0,0), sizes-uint32_t3(1));
}