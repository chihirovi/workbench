#pragma once

#include "../../Include/ColorSpace.h.slang"
#include "../../Include/VertexUtility.h.slang"
#include "../../Include/FetchTexture.h.slang"
#include "../../Include/Rt.h.slang"
#include "Types.h.slang"
#include "Lod.h.slang"

VertexType0Attribute fetch_hit_point_info_linear_albedo_lod(
    RayDesc ray,
    uint8_t as_lod,
    RtPayload payload,
    GPU_FrameRenderData frame_render_data_,
    Texture2D<float4> textures_[],
    SamplerState samplers_[]) {
    
    // instance
    Instance instance = frame_render_data_.instance_table[payload.render_instance_id_base + payload.render_instance_id];

    // material
    Material *material = &frame_render_data_.material_table[instance.material_id_base + instance.material_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;
    
    // barys
    float3 barys = float3((1.0 - payload.bary.x - payload.bary.y), payload.bary.x, payload.bary.y);
    
    // 
    VertexType0* raw_vertex_addr;
    uint32_t* raw_index_addr;
    get_as_lod_vertex_index_addr(as_lod, frame_render_data_, payload, raw_vertex_addr, raw_index_addr);

    // index
    uint32_t index0 = raw_index_addr[payload.triangle_idx*3 + 0];
    uint32_t index1 = raw_index_addr[payload.triangle_idx*3 + 1];
    uint32_t index2 = raw_index_addr[payload.triangle_idx*3 + 2];
    
    // uv
    float2 uv0 = unpackHalf2x16ToFloat(raw_vertex_addr[index0].packed_tex_coord);
    float2 uv1 = unpackHalf2x16ToFloat(raw_vertex_addr[index1].packed_tex_coord);
    float2 uv2 = unpackHalf2x16ToFloat(raw_vertex_addr[index2].packed_tex_coord);
    //float2 uv0 = float2(raw_vertex_addr[index0].tex_coord_f32[0], raw_vertex_addr[index0].tex_coord_f32[1]);
    //float2 uv1 = float2(raw_vertex_addr[index1].tex_coord_f32[0], raw_vertex_addr[index1].tex_coord_f32[1]);
    //float2 uv2 = float2(raw_vertex_addr[index2].tex_coord_f32[0], raw_vertex_addr[index2].tex_coord_f32[1]);

    float2 uv = barys[0]*uv0 + barys[1]*uv1 + barys[2]*uv2;
    
    // position
    float3 position = ray.Origin + payload.hit_t*ray.Direction;

    // albedo
    float4 material_bcf = float4(material.base_color_factor[0], material.base_color_factor[1], material.base_color_factor[2], material.base_color_factor[3]);

    float4 vertex_color0 = float4(float(raw_vertex_addr[index0].color[0])/255.0, float(raw_vertex_addr[index0].color[1])/255.0, float(raw_vertex_addr[index0].color[2])/255.0, float(raw_vertex_addr[index0].color[3])/255.0);
    float4 vertex_color1 = float4(float(raw_vertex_addr[index1].color[0])/255.0, float(raw_vertex_addr[index1].color[1])/255.0, float(raw_vertex_addr[index1].color[2])/255.0, float(raw_vertex_addr[index1].color[3])/255.0);
    float4 vertex_color2 = float4(float(raw_vertex_addr[index2].color[0])/255.0, float(raw_vertex_addr[index2].color[1])/255.0, float(raw_vertex_addr[index2].color[2])/255.0, float(raw_vertex_addr[index2].color[3])/255.0);

    float4 vertex_color = barys[0]*vertex_color0 + barys[1]*vertex_color1 + barys[2]*vertex_color2;
    vertex_color.xyz = srgb_to_linear(vertex_color.xyz);

    float4 albedo = fetch_texture_lod(
        frame_render_data_,
        textures_,
        samplers_,
        texture_view_id_base,
        texture_id_base,
        material->texture_view_id_albedo,
        uv,
        ALBEDO_TEX_DEFAULT_VALUE,
        1.0
    );
    albedo.xyz = srgb_to_linear(albedo.xyz);
    albedo = albedo * vertex_color * material_bcf;
    
    // normal
    float3 vertex_normal0 = decode_spherical_16(raw_vertex_addr[index0].encode_normal);
    float3 vertex_normal1 = decode_spherical_16(raw_vertex_addr[index1].encode_normal);
    float3 vertex_normal2 = decode_spherical_16(raw_vertex_addr[index2].encode_normal);
    float3 vertex_normal = barys[0]*vertex_normal0 + barys[1]*vertex_normal1 + barys[2]*vertex_normal2;

    float3x3 n_t_mat = mul(float32_t3x3(payload.blas_to_world_matrix), instance.local_reverse_transpose_3x3_model_matrix);
    vertex_normal = normalize(mul(n_t_mat, vertex_normal));

    // tangent
    float3 vertex_tangent0 = decode_spherical_16(raw_vertex_addr[index0].encode_tangent[0]);
    float3 vertex_tangent1 = decode_spherical_16(raw_vertex_addr[index1].encode_tangent[0]);
    float3 vertex_tangent2 = decode_spherical_16(raw_vertex_addr[index2].encode_tangent[0]);
    float3 vertex_tangent = barys[0]*vertex_tangent0 + barys[1]*vertex_tangent1 + barys[2]*vertex_tangent2;
    vertex_tangent = normalize(mul(float32_t3x3(payload.blas_to_world_matrix), mul(float32_t3x3(instance.model_matrix), vertex_tangent)));
    //vertex_tangent = normalize(mul(float32_t3x3(payload.blas_to_world_matrix), mul(float32_t3x3(instance.local_reverse_transpose_3x3_model_matrix), vertex_tangent)));
    
    float hand = raw_vertex_addr[index0].encode_tangent[1] == 1 ? 1.0 : -1.0;
    float3 bitangent = normalize(cross(vertex_normal, vertex_tangent))*hand;
    float32_t3x3 its = transpose(float32_t3x3(vertex_tangent, bitangent, vertex_normal));
    
    float3 normal = fetch_texture_lod(
        frame_render_data_,
        textures_,
        samplers_,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_normal,
        uv,
        NORMAL_TEX_DEFAULT_VALUE,
        1.0
    ).xyz;
    normal = normalize( (normal * float3(material->normal_scale, material->normal_scale, 1) * 2.0 - 1.0));
    normal = mul(its, normal);
    
    // metalness roughness
    float3 mr = fetch_texture_lod(
        frame_render_data_,
        textures_,
        samplers_,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_metallic_roughness, 
        uv,
        METALLIC_ROUGHNESS_TEX_DEFAULT_VALUE,
        1.0
    ).xyz;
    float metalness = mr.b * material->metallic_factor;
    float roughness = mr.g * material->roughness_factor;

    // emissive
    float4 emissive = fetch_texture_lod(
        frame_render_data_,
        textures_,
        samplers_,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_emissive, 
        uv,
        EMISSIVE_TEX_DEFAULT_VALUE,
        1.0
    );
    emissive.xyz = srgb_to_linear(emissive.xyz);
    float3 emissive_factor = float3(material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2]);
    float emissive_strength = material->emissive_factor[3];
    
    emissive = float4(emissive.xyz*emissive_factor*emissive_strength, 0.0);
    
    // return
    VertexType0Attribute vertex_attrib;
    vertex_attrib.position = position;
    vertex_attrib.normal = normal;
    vertex_attrib.albedo = albedo;
    vertex_attrib.metalness = metalness;
    vertex_attrib.roughness = roughness;
    vertex_attrib.emissive = emissive;
    vertex_attrib.is_double_siced = (material->is_double_side != 0 ? true : false);
    
    return vertex_attrib;
}

