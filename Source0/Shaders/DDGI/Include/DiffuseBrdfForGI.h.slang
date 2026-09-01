#pragma once

float Square(float v) { return v * v; }
float2 Square(float2 v) { return float2(v.x * v.x, v.y * v.y); }
float3 Square(float3 v) { return float3(v.x * v.x, v.y * v.y, v.z * v.z); }
float4 Square(float4 v) { return float4(v.x * v.x, v.y * v.y, v.z * v.z, v.w * v.w); }

float CosTheta(float3 w)
{
    return w.z;
}

float Cos2Theta(float3 w)
{
    return w.z * w.z;
}

float AbsCosTheta(float3 w)
{
    return abs(CosTheta(w));
}

float Sin2Theta(float3 w)
{
    return max(0.0f, 1.0f - Cos2Theta(w));
}

float SinTheta(float3 w)
{
    return sqrt(Sin2Theta(w));
}

float TanTheta(float3 w)
{
    return SinTheta(w) / CosTheta(w);
}

float Tan2Theta(float3 w)
{
    return Sin2Theta(w) / Cos2Theta(w);
}

float CosPhi(float3 w)
{
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1.0f : clamp(w.x / sinTheta, -1.0f, 1.0f);
}

float SinPhi(float3 w)
{
    float sinTheta = SinTheta(w);
    return (sinTheta == 0) ? 1.0f : clamp(w.z / sinTheta, -1.0f, 1.0f);
}

float Cos2Phi(float3 w)
{
    float cosPhi = CosPhi(w);
    return cosPhi * cosPhi;
}

float Sin2Phi(float3 w)
{
    float sinPhi = SinPhi(w);
    return sinPhi * sinPhi;
}


float3 fresnel_schlick(float3 f0, float3 f90, float cosPhi){
    return f0 + (f90 - f0) * pow(saturate(1.0 - cosPhi), 5.0); 
}

// Trowbridge-Reitz GGX normal distribution function
// Statistically approximates the ratio of microfacets aligned to some (halfway) vector h
//
float NormalDistributionTrowbridgeReitzGGXIsotropic(float3 wm, float alpha)
{
    float a2 = alpha * alpha;
    float NdotH2 = Cos2Theta(wm);

    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = 3.1415926535 * denom * denom;

    return nom / denom;
}

float NormalDistributionTrowbridgeReitzGGXAnisotropic(float3 wm, float ax, float ay)
{
    float dotHX2 = Square(wm.x);
    float dotHY2 = Square(wm.y);
    float cos2Theta = Cos2Theta(wm);
    float ax2 = Square(ax);
    float ay2 = Square(ay);

    return 1.0f / (3.1415926535 * ax * ay * Square(dotHX2 / ax2 + dotHY2 / ay2 + cos2Theta));
}

// Statistically approximates the ratio of microfacets that overshadow each other causing light rays to lose their energy in the process.
// https://schuttejoe.github.io/

// G1 and G2 naming note: 
// G2(L, V, M) is % visible in 2 directions
// G1(V, M) is % visible in just 1 direction
// In practice, G2 is derived from G1: G2(L, V, M) = G1(L, M) * G1(V, M)
// From https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf, slide 21

// Note on G2 meaning: to effectively approximate the geometry we need to take account 
// of both the view direction (geometry obstruction) and the light direction vector (geometry shadowing).
float HeightCorrelatedSmithGGXG2(float3 wo, float3 wi, float a)
{
    float absDotNV = AbsCosTheta(wo);
    float absDotNL = AbsCosTheta(wi);
    float a2 = a * a;

    // height-correlated masking function
    // https://twvideo01.ubm-us.net/o1/vault/gdc2017/Presentations/Hammon_Earl_PBR_Diffuse_Lighting.pdf
    float denomA = absDotNV * sqrt(a2 + (1.0f - a2) * absDotNL * absDotNL);
    float denomB = absDotNL * sqrt(a2 + (1.0f - a2) * absDotNV * absDotNV);

    return 2.0f * absDotNL * absDotNV / (denomA + denomB + 0.001); // add constant to avoid NaNs
}

float SeparableSmithGGXG1(float3 w, float3 wm, float ax, float ay)
{
    float absTanTheta = abs(TanTheta(w));

    if (isinf(absTanTheta))
        return 0.0f;

    float a = sqrt(Cos2Phi(w) * ax * ax + Sin2Phi(w) * ay * ay);
    float a2Tan2Theta = Square(a * absTanTheta);

    float lambda = 0.5f * (-1.0f + sqrt(1.0f + a2Tan2Theta));
    return 1.0f / (1.0f + lambda);
}

float SeparableSmithGGXG1(float3 w, float a)
{
    float a2 = a * a;
    float absDotNV = AbsCosTheta(w);

    return 2.0f / (1.0f + sqrt(a2 + (1 - a2) * absDotNV * absDotNV));
}

// Renormalized Disney diffuse formula to be energy conserving 
// https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf
// 3.1.3 Energy conservation
//

float3 FresnelSchlick(float3 f0, float3 f90, float cosPhi)
{
    // Clamp between 0 and 1, because input cos might be slightly greater 
    // than 1 due to accumulated errors which will leads pow to output NaNs
    return f0 + (f90 - f0) * pow(saturate(1.0 - cosPhi), 5.0); 
}

float DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = lerp(0, 0.5, linearRoughness);
    float energyFactor = lerp(1.0, 1.0 / 1.51, linearRoughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
    float3 f0 = float3(1.0f, 1.0f, 1.0f);
    float lightScatter = FresnelSchlick(f0, fd90, NdotL).r;
    float viewScatter = FresnelSchlick(f0, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor / 3.1415926535;
}

float3 DiffuseBRDFForGI(
    float3 viewDirection, 
    float3 albedo,
    float3 normal,
    float metalness,
    float roughness){
    // Since Disney diffuse if not uniform in all directions like Lambert,
    // ideally we would precompute irradiance with Disney diffuse term computed for each ray,
    // but instead we do it a bit hacky and apply single term to whole irradiance
    float3 L = normal;
    float3 H = normalize(L + viewDirection);
    float NdotV = dot(normal, viewDirection);
    float LdotH = dot(H, L);
    float NdotH = LdotH;

    float3 f0 = lerp(0.04, albedo, metalness);
    float3 F = FresnelSchlick(f0, 1.0, NdotH);
    float3 diffuse = DisneyDiffuse(NdotV, 1.0, LdotH, roughness) * (1.0 - metalness) * albedo * (1.0 - F);
    
    return diffuse;
}