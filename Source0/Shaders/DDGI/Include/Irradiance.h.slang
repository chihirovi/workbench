#pragma once

#include "Types.h.slang"
#include "ProbeIndex.h.slang"
#include "ProbeOctahedral.h.slang"
#include "ProbeTexture.h.slang"
#include "../../Include/SamplerType.h.slang"

float3 get_surface_bias(DDGIDesc desc, float3 normal, float3 camera_dir) {
    return float3(0.0);
    //return (normal * desc.view_normal_bias[1]) + (-camera_dir * desc.view_normal_bias[0]);
}

float3 get_probe_irradiance(
    DDGIDesc desc,
    float3 world_pos,
    float3 world_normal,
    float3 camera_dir,
    Texture2DArray probe_irradiance_tex,
    Texture2DArray probe_chit_t_tex,
    SamplerState[] samplers, 
    RWStructuredBuffer<float4> offsets){
    
    uint32_t3 base_probe_coords = get_base_probe_grid_coords(desc, world_pos);
    float3 base_probe_world_pos = probe_pos_from_index3D(desc, base_probe_coords, offsets);
    float3 surface_bias = get_surface_bias(desc, world_normal, camera_dir);
    float3 biased_world_position = (world_pos + surface_bias);
    float3 grid_space_distance = abs(biased_world_position - base_probe_world_pos);
    float3 alpha = clamp((grid_space_distance / desc.scales.xyz), float3(0.f), float3(1.f)); 

    float3 irradiance = float3(0.f);
    float accumulated_weights = 0.f;

    // Iterate over the 8 closest probes and accumulate their contributions
    for(uint probe_index = 0; probe_index < 8; probe_index++){

        // Compute the offset to the adjacent probe in grid coordinates by
        // sourcing the offsets from the bits of the loop index: x = bit 0, y = bit 1, z = bit 2
        uint32_t3 adjacent_probe_offset = uint32_t3(probe_index, probe_index >> 1, probe_index >> 2) & uint32_t3(1, 1, 1);

        // Get the 3D grid coordinates of the adjacent probe by adding the offset to 
        // the base probe and clamping to the grid boundaries
        uint32_t3 adjacent_probe_coords = clamp(base_probe_coords + adjacent_probe_offset, uint32_t3(0), desc.sizes.xyz - uint32_t3(1));

        // Get the adjacent probe's world position
        float3 adjacent_probe_world_position = probe_pos_from_index3D(desc, adjacent_probe_coords, offsets);

        // Compute the distance and direction from the (biased and non-biased) shading point and the adjacent probe
        float3 world_pos_to_adj_probe = normalize(adjacent_probe_world_position - world_pos);
        float3 biased_pos_to_adj_probe = normalize(adjacent_probe_world_position - biased_world_position);
        float biased_pos_to_adj_probe_dist = length(adjacent_probe_world_position - biased_world_position);

        // Compute trilinear weights based on the distance to each adjacent probe
        // to smoothly transition between probes. adjacentProbeOffset is binary, so we're
        // using a 1-alpha when adjacentProbeOffset = 0 and alpha when adjacentProbeOffset = 1.
        float3 trilinear = max(float3(0.001f), lerp(1.f - alpha, alpha, float3(adjacent_probe_offset)));
        float  trilinear_weight = (trilinear.x * trilinear.y * trilinear.z);

        // A naive soft backface weight would ignore a probe when
        // it is behind the surface. That's good for walls, but for
        // small details inside of a room, the normals on the details
        // might rule out all of the probes that have mutual visibility 
        // to the point. We instead use a "wrap shading" test. The small
        // offset at the end reduces the "going to zero" impact.
        float wrap_shading = (dot(world_pos_to_adj_probe, world_normal) + 1.f) * 0.5f; //con[-1, 1] -> [0, 1]
        float weight = (wrap_shading * wrap_shading) + 0.2f;

        //float weight = max(0.0, dot(world_pos_to_adj_probe, world_normal));
        //if(weight == 0.0) continue;
        
        // Get the texture coordinates for the octant of the probe
        float3 probe_distance_texture_UV = get_probe_texture_UV<DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER>(desc, adjacent_probe_coords, -world_pos_to_adj_probe);

        // Sample the probe's distance texture to get the mean distance to nearby surfaces
        float2 filtered_distance = probe_chit_t_tex.SampleLevel(samplers[int(SAMPLER_TYPE::DDGI_SAMPLER)], probe_distance_texture_UV, 0.0).rg;

        // Find the variance of the mean distance
        float variance = abs((filtered_distance.x * filtered_distance.x) - filtered_distance.y);

        // Occlusion test
        float chebyshev_weight = 1.f;
        if(biased_pos_to_adj_probe_dist > filtered_distance.x){ //occluded

            // v must be greater than 0, which is guaranteed by the if condition above.
            float v = biased_pos_to_adj_probe_dist - filtered_distance.x;
            chebyshev_weight = variance / (variance + (v * v));

            // Increase the contrast in the weight
            chebyshev_weight = max((chebyshev_weight * chebyshev_weight * chebyshev_weight), 0.f);
            //chebyshev_weight = max(chebyshev_weight, 0.f);
        }

        // Avoid visibility weights ever going all the way to zero because
        // when *no* probe has visibility we need a fallback value
        weight *= max(0.05f, chebyshev_weight);

        // Avoid a weight of zero
        weight = max(0.00001f, weight);

        // A small amount of light is visible due to logarithmic perception, so
        // crush tiny weights but keep the curve continuous
        const float crush_threshold = 0.2f;
        if (weight < crush_threshold){
            weight *= (weight * weight) * (1.f / (crush_threshold * crush_threshold));
        }

        // Apply the trilinear weights
        weight *= trilinear_weight;
        
        //sample the probe's irradiance
        float3 probe_irradiance_texture_UV = get_probe_texture_UV<DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER>(desc, adjacent_probe_coords, world_normal);
        float3 probe_irradiance = probe_irradiance_tex.SampleLevel(samplers[int(SAMPLER_TYPE::DDGI_SAMPLER)], probe_irradiance_texture_UV, 0.0).rgb;

        // gamma?
        float3 exponent = DDGI_IRRADIANCE_ENCODE_GAMMA*0.5;
        //probe_irradiance = pow(probe_irradiance, exponent);
        
        //accumulate the weighted irradiance
        irradiance += weight * probe_irradiance;
        accumulated_weights += weight;
    }
    
    if(accumulated_weights == 0.0) return float3(0.0);
    
    irradiance *= (1.f / accumulated_weights);
    //irradiance *= irradiance;

    return irradiance;
}