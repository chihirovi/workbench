#pragma once
#include "../../Include/Constant.h.slang"
#include "Lambert.h.slang"

float3 schlick_fresnel(float3 specular, float VH) {
    return specular + (1.0 - specular) * pow((1.0f - VH), 5.0f);
}

float D_beckmann(float m, float NH) {
    float c2 = NH * NH;
    float c4 = c2 * c2;
    float m2 = m * m;
    return (1.0f / (m2 * c4) * exp((-1.0f / m2) * (1.0f - c2) / c2));
}

float G2_vcavity(float NH, float NV, float NL, float VH) {
    return min(1.0f, min(2.0f * NH * NV / VH,  2.0f * NH * NL / VH));
}

float D_GGX(float m2, float NH) {
    float f = (NH * m2 - NH) * NH + 1;
    return m2/(M_PI * f * f);
}

float G2_smith(float NL, float NV, float m2) {
    float NL2 = NL * NL;
    float NV2 = NV * NV;
    float lambda_v = (-1.0f + sqrt(m2 * (1.0f - NL2) / NL2 + 1.0f)) * 0.5f;
    float lambda_l = (-1.0f + sqrt(m2 * (1.0f - NV2) / NV2 + 1.0f)) * 0.5f;
    return 1.0f / (1.0f + lambda_v + lambda_l);
}


float3 ggx_smith(float3 base_color, float3 specular, float roughness, float3 N, float3 L, float3 V) {

    float3 H = normalize(V+L);
    
    float NH = saturate(dot(N, H));
    float NV = saturate(dot(N, V));
    float NL = saturate(dot(N, L));
    float VH = saturate(dot(V, H));
    
    float a = roughness * roughness;
    float m2 = a * a;
    float D = D_GGX(m2, NH);
    float G2 = G2_smith(NH, NV, m2);
    float3 Fr = schlick_fresnel(specular, VH);
    
    float3 result = (D * G2 * Fr)/(4.0f * NV * NL);
    return saturate(result);
}


////////////////////////
float cosTheta(float3 w) { return w[1]; }
float cos2Theta(float3 w) { return w[1] * w[1]; }
float sinTheta(float3 w) { return sqrt(max(1.0f - cosTheta(w) * cosTheta(w), 0.0f)); }
float tanTheta(float3 w) { return sinTheta(w) / cosTheta(w); }
float tan2Theta(float3 w) { return tanTheta(w) * tanTheta(w); }

float cosPhi(float3 w) {
    float sintheta = sinTheta(w);
    if (sintheta == 0) return 1.0f;
    return clamp(w[0] / sintheta, -1.0f, 1.0f);
}

float sinPhi(float3 w) {
    float sintheta = sinTheta(w);
    if (sintheta == 0) return 0.0f;
    return clamp(w[2] / sintheta, -1.0f, 1.0f);
}

float cosPhi2(float3 w) { return cosPhi(w) * cosPhi(w); }
float sinPhi2(float3 w) { return sinPhi(w) * sinPhi(w); }


float G1(float3 v, float alpha){
    float delta = alpha * tanTheta(v);
    float result = 2.0f / (1.0f + sqrt(1.0f + delta * delta));
    return result;
}

float lambda(float3 v, float alpha){
    float delta = max(alpha * tanTheta(v), 0.0f);
    return max((-1.0f + sqrt(1.0f + delta * delta)) / 2.0f, 0.0f);
}

float G2(float3 o, float3 i, float alpha){
    return G1(o, alpha) * G1(i, alpha);
}

float D(float3 m, float alpha){
    const float tan2theta = tan2Theta(m);
    if (isinf(tan2theta)) return 0;
    const float cos4theta = cos2Theta(m) * cos2Theta(m);
    const float term = 1.0f + tan2theta / (alpha * alpha);
    return 1.0f / ((M_PI * alpha * alpha * cos4theta) * term * term);
}

float3 samplem(float u, float v, float alpha){
    float theta = atan(alpha * sqrt(v) / sqrt(max(1.0f - v, 0.0f)));
    float phi = 2.0f * M_PI * u;
    return float3(cos(phi) * sin(theta), cos(theta), sin(phi) * sin(theta));
}

float3 Fresnel(float3 F0, float im){
    float delta = max(1.0f - im, 0.0f);
    return F0 + (float3(1.0f) - F0) * pow(delta, 5);
}

void sampling_BSDF_GGX(float3 F0, float3 N, float3 wi, out float3 wo, float alpha, float2 rand){

    float3 i = wi;
    float3 m = samplem(rand[0], rand[1], alpha);
    float3 o = reflect(wi, m);
    if (wi[1] < 0.0f) {
        wo = float3(0.0);
        return;
    }
    
    float3 T, B;
    tangentSpace(N, T, B);
    o = normalize(T * o.x + B * o.y + N * o.z);
    wo = o;
    
    /*
    float im = abs(dot(i, m));
    float in = abs(dot(i, N));
    float on = abs(dot(o, N));

    float3 F = Fresnel(F0, im);
    float G_ = G2(i, o, alpha);
    float D_ = D(m, alpha);

    float3 brdf = F * G_ * D_ / (4.0f * in * on);

    pdf = D_ * cosTheta(m) / (4.0f * abs(dot(m, o)));
    return brdf;
    */
}

float pdf_BSDF_GGX(float alpha, float3 dir, float3 wi) {
    
    float3 m = (dir + wi)/2.0f;
    float D_ = D(m, alpha);
    float pdf = D_ * cosTheta(m) / (4.0f * abs(dot(m, dir)));
    return pdf;
}

float3 brdf_BSDF_GGX(float3 F0, float3 N, float3 wi, float3 dir, float alpha) {

    float3 i = wi;
    float3 m = (wi+dir)/2.0f;

    float im = abs(dot(i, m));
    float in = abs(dot(i, N));
    float on = abs(dot(dir, N));

    float3 F = Fresnel(F0, im);
    float G_ = G2(i, dir, alpha);
    float D_ = D(m, alpha);
    float3 brdf = F * G_ * D_ / (4.0f * in * on);
    //float3 brdf = F * G_ / (4.0f);
    return brdf;
}

float3 sampleGGX(float3 N, float3 V, float roughness, float2 xi){
    float a = roughness * roughness;

    // --- 半ベクトルの極座標サンプリング ---
    float phi = 2.0 * M_PI * xi.x;
    float cosTheta = sqrt((1.0 - xi.y) / (1.0 + (a * a - 1.0) * xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    // --- 接空間での半ベクトル ---
    float3 Ht = float3(sinTheta * cos(phi), sinTheta * sin(phi), cosTheta);

    // --- 法線Nに対する接空間を構築 ---
    float3 T, B;
    if (abs(N.z) < 0.999)
        T = normalize(cross(N, float3(0,0,1)));
    else
        T = normalize(cross(N, float3(0,1,0)));
    B = cross(T, N);

    // --- 半ベクトルをワールド空間へ ---
    float3 H = normalize(T * Ht.x + B * Ht.y + N * Ht.z);

    // --- 半ベクトルから反射方向を生成 ---
    float3 L = reflect(-V, H);
    return normalize(L);
}

float ggxPdf(float3 N, float3 V, float3 L, float roughness){
    float3 H = normalize(V + L);
    float NoH = saturate(dot(N, H));
    float VoH = saturate(dot(V, H));

    float a = roughness * roughness;
    float a2 = a * a;

    float denom = (NoH * NoH) * (a2 - 1.0) + 1.0;
    float D = a2 / (M_PI * denom * denom);

    float pdf = D * NoH / (4.0 * VoH);
    return pdf;
}