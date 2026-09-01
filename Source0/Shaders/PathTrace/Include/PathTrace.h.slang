#pragma once
#include "../../Include/ColorSpace.h.slang"
#include "../../DDGI/Include/FetchHitPointInfo.h.slang"
#include "../Include/Util.h.slang"
#include "../Include/Random.h.slang"
#include "../Include/Lambert.h.slang"
#include "Bsdf.h.slang"

float3 path_trace(
    RayDesc ray_desc, 
    inout UniformRandom uniform_random, 
    RaytracingAccelerationStructure as, 
    GPU_FrameRenderData frame_data,
    Texture2D<float4> texs[],
    SamplerState sampler_stats[]) {

    float3 radiance = float3(0.0);
    float3 alpha = float3(1.0); // fs * cos / pdf
 
    for(int ray_idx = 0; ray_idx < 8; ray_idx++){

        RtPayload payload;

        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
        
        // miss
        if(payload.is_hit == false) break;
        
        // hit
        VertexType0Attribute vertex_attribute = fetch_hit_point_info_linear_albedo_no_lod(
            ray_desc,
            0,
            payload,
            frame_data,
            texs,
            sampler_stats 
        );
        
        // back face check
        bool is_back_face_hit = dot(-ray_desc.Direction, vertex_attribute.normal) <= 0.0;
        if(is_back_face_hit) vertex_attribute.normal *= -1.0;
        
        //trace ray, ω_oは出射方向，ω_iは入射方向, 逆向きにトレースしてることに注意
        
        float3 omega_o = -ray_desc.Direction;  
        float3 Le = vertex_attribute.emissive.rgb;
        float3 omega_i;

        // bsdf
        BSDF bsdf = BSDF(vertex_attribute.roughness, vertex_attribute.metalness, vertex_attribute.normal, vertex_attribute.albedo.rgb, -ray_desc.Direction);
        bsdf.sample_fs(float4(uniform_random.rand(), uniform_random.rand()), omega_i);
        if (dot(omega_i, vertex_attribute.normal) <= 0) break;
             
        //update radiance += alpha*Le(x,ω_i,ω_o)
        radiance += alpha*Le;

        //update alpha (alpha = fs * cos / pdf)
        alpha *= bsdf.eval_fs(omega_i) * dot(vertex_attribute.normal, omega_i) * 1.0/bsdf.eval_pdf(omega_i);
        
        //next ray
        ray_desc.Origin         = vertex_attribute.position;
        ray_desc.Direction      = omega_i;
    }

    return radiance;
}