#pragma once
#include "../../Include/ColorSpace.h.slang"
#include "../../DDGI/Include/FetchHitPointInfo.h.slang"
#include "../Include/Util.h.slang"
#include "../Include/Random.h.slang"
#include "../Include/Lambert.h.slang"
#include "AreaLight.h.slang"
#include "Bsdf.h.slang"

float3 path_trace_nee(
    RayDesc ray_desc, 
    inout UniformRandom uniform_random, 
    RaytracingAccelerationStructure as, 
    GPU_FrameRenderData frame_data,
    Texture2D<float4> texs[],
    SamplerState sampler_stats[],
    inout AreaLightSampler area_light_sampler) {

    float3 radiance = float3(0.0);
    float3 alpha = float3(1.0); // fs * cos / pdf
    
    // calc first hit 
    RtPayload payload;

    TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
    
    //      miss
    if(payload.is_hit == false) return radiance;
    
    //      hit
    VertexType0Attribute hit_surface= fetch_hit_point_info_linear_albedo_no_lod(
        ray_desc,
        0,
        payload,
        frame_data,
        texs,
        sampler_stats 
    );
    
    //      back face check
    bool is_back_face_hit = dot(-ray_desc.Direction, hit_surface.normal) <= 0.0;
    if(is_back_face_hit) hit_surface.normal *= -1.0;
    
    // first hit
    radiance += hit_surface.emissive.rgb;

    for(int ray_idx = 0; ray_idx < 8; ray_idx++){

        BSDF bsdf = BSDF(hit_surface.roughness, hit_surface.metalness, hit_surface.normal, hit_surface.albedo.rgb, -ray_desc.Direction);

        // NEE
        
        //      light sampling
        float3 light_pos;
        float light_pdf;
        float3 light_normal;
        float3 nee_Le;
        area_light_sampler.sample(float4(uniform_random.rand(), uniform_random.rand()).xyz, light_pos, light_pdf, light_normal, nee_Le);

        //      shadow ray
        RayDesc nee_ray_desc;
        nee_ray_desc.Origin = hit_surface.position;
        nee_ray_desc.Direction = normalize(light_pos - nee_ray_desc.Origin);
        nee_ray_desc.TMin = 1e-2;
        nee_ray_desc.TMax = length(light_pos - nee_ray_desc.Origin) - nee_ray_desc.TMin;
        RtPayload nee_payload;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, nee_ray_desc, nee_payload);
        
        //      nee radiance
        float V = nee_payload.is_hit == false ? 1.0 : 0.0;
        float G = abs(dot(hit_surface.normal, nee_ray_desc.Direction)) * abs(dot(light_normal, nee_ray_desc.Direction)) / dot(light_pos-hit_surface.position, light_pos-hit_surface.position);
        // update radiance += alpha * fs * G * V * 1/pdf *Le(x,ω_i,ω_o)
        radiance += alpha * bsdf.eval_fs(nee_ray_desc.Direction) * G * V * 1.0f/light_pdf * nee_Le;
        
        
        // Next ray
        
        //      trace ray, ω_oは出射方向，ω_iは入射方向, 逆向きにトレースしてることに注意
        float3 omega_i;

        //      bsdf
        bsdf.sample_fs(float4(uniform_random.rand(), uniform_random.rand()), omega_i);
        if (dot(omega_i, hit_surface.normal) <= 0) break;

        //      update alpha (alpha = fs * cos / pdf)
        alpha *= bsdf.eval_fs(omega_i) * dot(hit_surface.normal, omega_i) * 1.0f/bsdf.eval_pdf(omega_i);

        //      next ray
        ray_desc.Origin         = hit_surface.position;
        ray_desc.Direction      = omega_i;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
        
        //  miss
        if(payload.is_hit == false) break;
        
        //  hit
        hit_surface = fetch_hit_point_info_linear_albedo_no_lod(
            ray_desc,
            0,
            payload,
            frame_data,
            texs,
            sampler_stats 
        );
        
        //  back face check
        bool is_back_face_hit = dot(-ray_desc.Direction, hit_surface.normal) <= 0.0;
        if(is_back_face_hit) hit_surface.normal *= -1.0;
    }

    return radiance;
}
