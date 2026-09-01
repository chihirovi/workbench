#pragma once
#include "Random.h.slang"
#include "../../Include/Rt.h.slang"

void tangentSpace(float3 N, out float3 T, out float3 B){
    float s = (N.z >= 0.0f) ? 1.0f : -1.0f;
    float a = -1.0f / (s + N.z);
    float b = N.x*N.y*a;
    T = float3(1.0f+s*N.x*N.x*a, s*b, -s*N.x);
    B = float3(b, s+N.y*N.y*a, -N.y);
}

float3 sampleCosineHemisphere(float3 N, float2 rand){
    float2 tmp = rand;
    float u1 = tmp.x;
    float u2 = tmp.y;
    
    float r = sqrt(u1);
    float phi = 2.0f * 3.14159265 * u2;
    
    float3 H;
    H.x = r * cos(phi);
    H.y = r * sin(phi);
    H.z = sqrt(1-u1);
    
    float3 T, B;
    tangentSpace(N, T, B);
    
    return normalize(T*H.x + B*H.y + N*H.z);
}

float cosinePDF(float3 N, float3 dir){
    return dot(N,dir)/3.1415926535;
}

//ω_oは出射方向，ω_iは入射方向, 逆向きにトレースしてることに注意
void evaluate_lambert(
    in VertexType0Attribute hit_payload,
    in float3 omega_o,
    inout UniformRandom uniform_random,
    out float3 out_Le,
    out float3 out_omega_i,
    out float3 out_alpha){
    
    
    //Le(x,ω_i,ω_o)
    out_Le = hit_payload.emissive.rgb;   
    
    //sample ω_i
    out_omega_i = sampleCosineHemisphere(hit_payload.normal, uniform_random.rand());
    
    //update alpha(Leを除いた部分), (= fs * cos / pdf)
    // cosinePDF = cos/pi;
    //alpha = albedo/pi * cos / cosinePDF; -> albedo
    out_alpha = hit_payload.albedo.rgb;
}