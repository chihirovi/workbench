#pragma once

#include "../../Include/GlobalDescriptor.h.slang"
#include "../../Include/ColorSpace.h.slang"
#include "../../Include/FetchTexture.h.slang"
#include "../../Include/Rt.h.slang"
#include "Rand.h.slang"
#include "LightSampleDesc.h.slang"
#include "BSDF.h.slang"
#include "Reservoir.h.slang"

float luminance(float3 c){
    return dot(c, float3(0.2126, 0.7152, 0.0722));
}

#define RAY_MAX_DEPTH 8
#define RAY_MIN_DEPTH 3

float3 path_trace_nee_sris(
    RayDesc ray_desc, 
    inout Random random, 
    RaytracingAccelerationStructure as, 
    GPU_FrameRenderData frame_data,
    Texture2D<float4> texs[],
    SamplerState sampler_stats[],
    float Le_scale) {

    float3 radiance = float3(0.0);
    float3 alpha = float3(1.0); // fs * cos / pdf
    
    // calc first hit 
    RtPayload payload;

    TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
    
    // miss
    if(payload.is_hit == false) return radiance;
    
    // hit
    VertexType0Attribute hit_surface = fetch_hit_point_info_linear_albedo_no_lod(
        ray_desc,
        0,
        payload,
        frame_data,
        texs,
        sampler_stats 
    );
    
    // back face check
    bool is_back_face_hit = dot(-ray_desc.Direction, hit_surface.normal) <= 0.0;
    if(is_back_face_hit) hit_surface.normal *= -1.0;
    
    // first hit
    radiance += hit_surface.emissive.rgb;

    for(int ray_idx = 0; ray_idx < RAY_MAX_DEPTH; ray_idx++){

        BSDF bsdf = BSDF(
            hit_surface.roughness, 
            hit_surface.metalness, 
            hit_surface.specular, 
            hit_surface.normal, 
            hit_surface.albedo.rgb, 
            -ray_desc.Direction
        );

        // NEE + sris

        // sris light sampling
        Reservoir<LightSample> reservoir = Reservoir<LightSample>();
        uint32_t M = 16;
        for (int i = 0; i < M; i++) {

            LightSample light_sample = light_samples_4096[uint32_t(random.Get() * 4096.f)];

            float3 light_dir = normalize(light_sample.position - hit_surface.position);

            // Fs * V * G * Le
            float3 Fs = bsdf.eval_fs(light_dir);
            float G = max(0, dot(hit_surface.normal, light_dir)) * max(0, dot(light_sample.normal, -light_dir)) 
                / dot(light_sample.position - hit_surface.position, light_sample.position - hit_surface.position);
            float3 Le = light_sample.emittanse;
            float p_hat = (Fs.r + Fs.g + Fs.b) * G * (Le.r + Le.g + Le.b);
            //float p_hat = length(Fs) * G * luminance(Le);
            float inv_q = light_sample.inv_pdf;
            
            reservoir.update(light_sample, p_hat, p_hat*inv_q, 1, random.Get());
        }
        if(reservoir.p_hat_y > 0.0f)
            reservoir.ucw = 1.0f / reservoir.p_hat_y * 1.0f / M * reservoir.weight_sum;
        
        //      shadow ray
        RayDesc nee_ray_desc;
        nee_ray_desc.Origin = hit_surface.position;
        nee_ray_desc.Direction = normalize(reservoir.y.position - nee_ray_desc.Origin);
        nee_ray_desc.TMin = 1e-5;
        nee_ray_desc.TMax = length(reservoir.y.position - nee_ray_desc.Origin) - nee_ray_desc.TMin;
        RtPayload nee_payload;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, nee_ray_desc, nee_payload);
        
        // nee radiance
        float3 light_dir = normalize(reservoir.y.position - hit_surface.position);

        float V = nee_payload.is_hit == false ? 1.0 : 0.0;
        float3 Fs = bsdf.eval_fs(light_dir);
        float G = max(0, dot(hit_surface.normal, light_dir)) * max(0, dot(reservoir.y.normal, -light_dir)) 
            / dot(reservoir.y.position - hit_surface.position, reservoir.y.position - hit_surface.position);
        float3 Le = reservoir.y.emittanse * Le_scale;

        radiance += alpha * Fs * V * G * Le * reservoir.ucw;
        
        // Next ray
        
        // trace ray, ω_oは出射方向，ω_iは入射方向, 逆向きにトレースしてることに注意
        float3 omega_i = bsdf.sample_fs(float4(random.Get(), random.Get(), random.Get(), random.Get()));
        if (dot(omega_i, hit_surface.normal) <= 0) break;

        // update alpha (alpha = fs * cos / pdf)
        alpha *= bsdf.eval_fs(omega_i) * dot(hit_surface.normal, omega_i) * 1.0f/bsdf.eval_pdf(omega_i);
        
        // Russian Roulette
        if(RAY_MIN_DEPTH <= ray_idx){
            float p = min(max3(alpha.x, alpha.y, alpha.z), 0.95);
            if(random.Get() > p) break;
            
            alpha /= p;
        }

        // next ray
        ray_desc.Origin         = hit_surface.position;
        ray_desc.Direction      = omega_i;
        TraceRay(as, RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
        
        // miss
        if(payload.is_hit == false) break;
        
        // hit
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
