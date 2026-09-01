#pragma once
#include "Material.h.slang"
#include "TextureView.h.slang"
#include "GPU_FrameRenderData.h.slang"

#define ALBEDO_TEX_DEFAULT_VALUE                float4(1.0, 1.0, 1.0, 1.0)
#define NORMAL_TEX_DEFAULT_VALUE                float4(0.5, 0.5, 1.0, 0.0)
#define OCCLUSION_TEX_DEFAULT_VALUE             float4(0.0, 0.0, 0.0, 0.0)
#define METALLIC_ROUGHNESS_TEX_DEFAULT_VALUE    float4(0.0, 1.0, 1.0, 0.0) //(none, roughness, metalness, none) factorに影響が出ないように,両方1.0 1.0がいい
#define EMISSIVE_TEX_DEFAULT_VALUE              float4(1.0, 1.0, 1.0, 1.0) //同じようにfactorに影響が出ないように1にしておく
#define SPECULAR_GLOSSINESS_TEX_DEFAULT_VALUE   float4(1.0, 1.0, 1.0, 1.0)

float4 fetch_texture(
    GPU_FrameRenderData frame_render_data_,
    Texture2D<float4> textures_[],
    SamplerState samplers_[],
    uint32_t texture_view_id_base, 
    uint32_t texture_id_base,
    int32_t texture_view_id,
    float2 texcoord,
    float4 default_value) {

    float4 texture_value = default_value;

    if (texture_view_id >= 0) {
        TextureView texture_view = frame_render_data_.texture_view_table[texture_view_id_base + texture_view_id];
        texture_value = textures_[NonUniformResourceIndex(texture_id_base+texture_view.texture_id)].Sample(
            samplers_[NonUniformResourceIndex(texture_view.sampler_type)],
            texcoord
        );
    }

    return texture_value;
}

float4 fetch_texture_grad(
    GPU_FrameRenderData frame_render_data_,
    Texture2D<float4> textures_[],
    SamplerState samplers_[],
    uint32_t texture_view_id_base, 
    uint32_t texture_id_base,
    int32_t texture_view_id,
    float2 texcoord,
    float2 ddx,
    float2 ddy,
    float4 default_value) {

    float4 texture_value = default_value;

    if (texture_view_id >= 0) {
        TextureView texture_view = frame_render_data_.texture_view_table[texture_view_id_base + texture_view_id];
        texture_value = textures_[NonUniformResourceIndex(texture_id_base+texture_view.texture_id)].SampleGrad(
            samplers_[NonUniformResourceIndex(texture_view.sampler_type)],
            texcoord,
            ddx,
            ddy
        );
    }

    return texture_value;
}

float4 fetch_texture_lod(
    GPU_FrameRenderData frame_render_data_,
    Texture2D<float4> textures_[],
    SamplerState samplers_[],
    uint32_t texture_view_id_base, 
    uint32_t texture_id_base,
    int32_t texture_view_id,
    float2 texcoord,
    float4 default_value,
    float mip_level) {

    float4 texture_value = default_value;

    if (texture_view_id >= 0) {

        TextureView texture_view = frame_render_data_.texture_view_table[texture_view_id_base + texture_view_id];

        texture_value = textures_[NonUniformResourceIndex(texture_id_base+texture_view.texture_id)].SampleLevel(
            samplers_[NonUniformResourceIndex(texture_view.sampler_type)],
            texcoord,
            mip_level
        );
    }

    return texture_value;
}
