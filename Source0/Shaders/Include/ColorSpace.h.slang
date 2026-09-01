#pragma once


float3 less_than(float3 f, float value){
    return float3(
        (f.x < value) ? 1.f : 0.f,
        (f.y < value) ? 1.f : 0.f,
        (f.z < value) ? 1.f : 0.f);
}

float3 linear_to_srgb(float3 rgb){
    rgb = clamp(rgb, 0.f, 1.f);
    return lerp(
        pow(rgb * 1.055f, 1.f / 2.4f) - 0.055f,
        rgb * 12.92f,
        less_than(rgb, 0.0031308f)
    );
}

float3 srgb_to_linear(float3 rgb){
    rgb = clamp(rgb, 0.f, 1.f);
    return lerp(
        pow((rgb + 0.055f) / 1.055f, 2.4f),
        rgb / 12.92f,
        less_than(rgb, 0.04045f)
    );
}

float3 ACESFilm(float3 x){
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x*(a*x + b)) / (x*(c*x + d) + e));
}