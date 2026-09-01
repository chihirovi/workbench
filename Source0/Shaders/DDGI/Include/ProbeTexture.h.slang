#pragma once

#include "Types.h.slang"
#include "ProbeIndex.h.slang"
#include "ProbeOctahedral.h.slang"


float3 get_probe_texture_UV<let NUM_TEXELS_INCLUDE_BORDER : uint32_t>(DDGIDesc desc, uint32_t3 id, float3 dir){

    float2 tex_coords = float2(float(id.x), float(id.y));
    float2 uv = tex_coords*float(NUM_TEXELS_INCLUDE_BORDER) + float(NUM_TEXELS_INCLUDE_BORDER)*0.5;

    float2 norm_uv = get_normalized_octahedral_coordinates_from_direction(dir);
    uv += norm_uv*float2(float(NUM_TEXELS_INCLUDE_BORDER-2)*0.5);
    
    
    uv /= float2(
        float(NUM_TEXELS_INCLUDE_BORDER * desc.sizes.x), 
        float(NUM_TEXELS_INCLUDE_BORDER * desc.sizes.y)
    );
    
    return float3(uv, float(id.z));
}