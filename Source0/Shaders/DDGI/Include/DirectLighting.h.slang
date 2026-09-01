#pragma once
#include "../../Include/GPU_FrameRenderData.h.slang"
#include "../../Include/Rt.h.slang"
#include "../../Include/Constant.h.slang"
#include "Types.h.slang"

float3 evaluate_direct_light(
    float3 world_pos,
    float3 world_normal,
    uint8_t lod,
    RaytracingAccelerationStructure bvh,
    GPU_FrameRenderData frame_render_data_) {
    
    float3 light_dir = frame_render_data_.directional_light_dir.xyz;
    
    RayDesc ray_desc;
    ray_desc.Direction = light_dir;
    ray_desc.Origin = world_pos;
    ray_desc.TMin = 1e-4;
    ray_desc.TMax = 1e5;
    
    RtPayload payload;
    payload.is_hit = true;
    payload.as_lod = lod;
    TraceRay(bvh, RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, 0xFF, 0, 0, 0, ray_desc, payload);
    
    float light_visiblity = payload.is_hit ? 0.0 : 1.0;
    float nol = max(dot(world_normal, light_dir), 0.0);
    
    float3 light_radiance = frame_render_data_.directional_light_rgba.rgb;
    float3 color = light_radiance
                *frame_render_data_.directional_light_rgba.a
                *nol
                *light_visiblity;

    return color;
}

float3 direct_diffuse_lighting(
    float3 world_pos,
    float3 world_normal,
    float3 albedo,
    uint8_t lod,
    RaytracingAccelerationStructure bvh,
    GPU_FrameRenderData frame_render_data_) {
    
    float3 brdf = albedo/M_PI;
    float3 lighting = float3(0.0);
    
    lighting += evaluate_direct_light(world_pos, world_normal, lod, bvh, frame_render_data_);
    
    return (brdf * lighting); //brdf = radiance/irradiance
}