#pragma once

#include "Types.h.slang"
#include "../../Include/Constant.h.slang"

/**
 * Computes a low discrepancy spherically distributed direction on the unit sphere,
 * for the given index in a set of samples. Each direction is unique in
 * the set, but the set of directions is always the same.
 */
float3 spherical_fibonacci(float sampleIndex, float numSamples){

    const float b = (sqrt(5.f) * 0.5f + 0.5f) - 1.f;
    float phi = M_PI * 2.0 * fract(sampleIndex * b);
    float cosTheta = 1.f - (2.f * sampleIndex + 1.f) * (1.f / numSamples);
    float sinTheta = sqrt(clamp(1.f - (cosTheta * cosTheta), 0.0, 1.0));

    return float3((cos(phi) * sinTheta), (sin(phi) * sinTheta), cosTheta);
}

/**
 * Rotate vector v with quaternion q.
 */
float3 quaternion_rotate(float3 v, float4 q){
    float3 b = q.xyz;
    float b2 = dot(b, b);
    return float3(v * (q.w * q.w - b2) + b * (dot(v, b) * 2.f) + cross(b, v) * (q.w * 2.f));
}

/**
 * Computes a spherically distributed, normalized ray direction for the given ray index in a set of ray samples.
 * Applies the volume's random probe ray rotation transformation to "non-fixed" ray direction samples.
 */
float3 get_probe_ray_direction(DDGIDesc desc, uint rayIndex){

    float sampleIndex = float(rayIndex);
    float numRays = DDGI_PROBE_NUM_RAY;

    // Get a ray direction on the sphere
    float3 direction = spherical_fibonacci(sampleIndex, numRays);

    // Apply a random rotation and normalize the direction
    return normalize(quaternion_rotate(direction, desc.probe_ray_rotation));
}