#pragma once

/**
 * Computes normalized octahedral coordinates for the given texel coordinates.
 * Maps the top left texel to (-1,-1).
 * Used by DDGIProbeBlendingCS() in ProbeBlending.hlsl.
 */
float2 get_normalized_octahedral_coordinates(uint32_t2 tex_coords, uint32_t num_texels){

    // Map 2D texture coordinates to a normalized octahedral space
    //vec2 octahedralTexelCoord = vec2(texCoords.x % numTexels, texCoords.y % numTexels);
    float2 octahedral_texel_coord = float2(tex_coords.x, tex_coords.y);

    // Move to the center of a texel
    octahedral_texel_coord.xy += 0.5f;

    // Normalize
    octahedral_texel_coord.xy /= float(num_texels);

    // Shift to [-1, 1);
    octahedral_texel_coord *= 2.f;
    octahedral_texel_coord -= float2(1.f, 1.f);

    return octahedral_texel_coord;
}

/**
 * Returns either -1 or 1 based on the sign of the input value.
 * If the input is zero, 1 is returned.
 */
float sign_not_zero(float v){
    return (v >= 0.f) ? 1.f : -1.f;
}

float2 sign_not_zero2(float2 v){
    return float2(sign_not_zero(v.x), sign_not_zero(v.y));
}

/**
 * Computes the normalized octahedral direction that corresponds to the
 * given normalized coordinates on the [-1, 1] square.
 * The opposite of DDGIGetOctahedralCoordinates().
 * Used by DDGIProbeBlendingCS() in ProbeBlending.hlsl.
 */
float3 get_direction_from_normalized_octahedral_coordinates(float2 norm_coords){

    float3 direction = float3(norm_coords.x, norm_coords.y, 1.f - abs(norm_coords.x) - abs(norm_coords.y));
    if (direction.z < 0.f){
        direction.xy = (1.f - abs(direction.yx)) * sign_not_zero2(direction.xy);
    }
    return normalize(direction);
}

/**
 * Computes the octant coordinates in the normalized [-1, 1] square, for the given a unit direction vector.
 * The opposite of DDGIGetOctahedralDirection().
 * Used by GetDDGIVolumeIrradiance() in Irradiance.hlsl.
 */
float2 get_normalized_octahedral_coordinates_from_direction(float3 direction){

    float l1norm = abs(direction.x) + abs(direction.y) + abs(direction.z);
    float2 norm_uv = direction.xy * (1.f / l1norm);
    if (direction.z < 0.f){
        norm_uv = (1.f - abs(norm_uv.yx)) * sign_not_zero2(norm_uv.xy);
    }
    return norm_uv;
}

// if z == 0 
float get_normalized_octahedral_coordinates_from_direction2(float2 direction){

    float l1norm = abs(direction.x) + abs(direction.y);
    float norm_u = direction.x * (1.f / l1norm);
    return norm_u;
}