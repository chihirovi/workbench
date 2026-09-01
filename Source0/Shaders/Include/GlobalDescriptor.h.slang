#pragma once 
#include "GPU_FrameRenderData.h.slang"
#include "Material.h.slang"
#include "SamplerType.h.slang"
#include "../DDGI/Include/Types.h.slang"

/*
* global descriptorはset number 4,5,6,7だけを使うように,
* T* address, address[i]は普通にアクセスすればいい
* descripor indexの Texture2D textures[] とかは，textures[nonuniform(i)]でアクセス
*/

// frame render data
[[vk_binding(0, 4)]]
ConstantBuffer<GPU_FrameRenderData, ScalarDataLayout> frame_render_data;

// testures
[[vk_binding(1, 4)]]
Texture2D<float4> textures[];

[[vk_binding(2, 4)]]
SamplerState samplers[MAX_SAMPLER];

// GBuffers
[[vk_binding(3, 4)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> albedo_gb;

[[vk_binding(4, 4)]]
[[vk_image_format("rgba32f")]]
RWTexture2D<float4> world_position_gb;

[[vk_binding(5, 4)]]
[[vk_image_format("rgba16f")]]
RWTexture2D<float4> normal_occ_gb;

[[vk_binding(6, 4)]]
[[vk_image_format("rg8")]]
RWTexture2D<float4> metal_rough_gb;

[[vk_binding(7, 4)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> emissive_gb;

[[vk_binding(8, 4)]]
[[vk_image_format("rgba16f")]]
RWTexture2D<float4> diffuse_irradiance_shadow_gb;

[[vk_binding(9, 4)]]
RWStructuredBuffer<float> screen_shot_buffer;

// DDGI
[[vk_binding(0, 5)]]
RaytracingAccelerationStructure accelaration_structures[5];

[[vk_binding(1, 5)]]
ConstantBuffer<DDGIDesc> ddgi_desc;

[[vk_binding(2, 5)]]
Texture2DArray<float4> probe_irradiance_texture;

[[vk_binding(3, 5)]]
//[[vk_image_format("r11f_g11f_b10f")]]
[[vk_image_format("rgba16f")]]
RWTexture2DArray<float4> probe_irradiance_texture_general;

[[vk_binding(4, 5)]]
Texture2DArray<float4> probe_chit_t_texture;

[[vk_binding(5, 5)]]
[[vk_image_format("rg16f")]]
RWTexture2DArray<float4> probe_chit_t_texture_general;

[[vk_binding(6, 5)]]
RWStructuredBuffer<float4> probe_trace_results;

[[vk_binding(7, 5)]]
RWStructuredBuffer<float4> probe_relocation_offset; 






