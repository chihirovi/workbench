#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/FetchTexture.h.slang"

#include "./Include/payload.h.slang"

[shader("fragment")]
FragOut main_fragment(
    in Mesh2FragVertexPayload vertex_payload,
    in Mesh2FragPrimitivePayloadFragment primitive_payload){

    FragOut frag_out;

    Material *material = &frame_render_data.material_table[primitive_payload.global_material_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;

    // albedo
    float4 albedo = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base,
        texture_id_base,
        material->texture_view_id_albedo,
        vertex_payload.tex_coord, 
        ALBEDO_TEX_DEFAULT_VALUE
    );
    if (albedo.a < material.alpha_cut_off) discard;
    albedo.xyz = srgb_to_linear(albedo.xyz);
    albedo = albedo * vertex_payload.color;
    frag_out.albedo_out = albedo;
    
    // world position
    frag_out.world_position_out = float4(vertex_payload.world_position.xyz, vertex_payload.clip_position.z);

    // normal occ
    float3 normal = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_normal,
        vertex_payload.tex_coord, 
        NORMAL_TEX_DEFAULT_VALUE
    ) .xyz;

    // ラスタライザでの法線補完では，長さは保たれない （線形補間なので)
    float3 N = normalize(vertex_payload.normal);
    float3 T = normalize(vertex_payload.tangent.xyz);
    float3 bitangent = normalize(cross(N, T))*vertex_payload.tangent.w;
    // tangent space->world space matrix
    float32_t3x3 its = transpose(float32_t3x3(T, bitangent, N));
    //float32_t3x3 its = float32_t3x3(T, bitangent, N);
    normal = normalize((normal * float3(material->normal_scale, material->normal_scale, 1) * 2.0 -1.0));
    normal = mul(its, normal);
    normal *= primitive_payload.is_front_face ? 1.0 : -1.0;
    
    
    float occlusion = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_occlusion, 
        vertex_payload.tex_coord,
        OCCLUSION_TEX_DEFAULT_VALUE
    ).x;
    // 0.0 は完全遮蔽，1.0は遮蔽されていない
    occlusion = 1.0 + material->occlusion_strength * (occlusion - 1.0);
    frag_out.normal_occ_out = float4(normal, occlusion);
    
    // metal rough
    float3 mr = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_metallic_roughness, 
        vertex_payload.tex_coord,
        METALLIC_ROUGHNESS_TEX_DEFAULT_VALUE
    ).xyz;
    float metalness = mr.b * material->metallic_factor;
    float roughness = mr.g * material->roughness_factor;

    frag_out.metal_rough_out = float4(metalness, roughness, 0.0, 0.0);
    
    // emissive
    float4 emissive = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_emissive, 
        vertex_payload.tex_coord,
        EMISSIVE_TEX_DEFAULT_VALUE
    );
    emissive.xyz = srgb_to_linear(emissive.xyz);
    float3 emissive_factor = float3(material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2]);
    emissive_factor = srgb_to_linear(emissive_factor);
    frag_out.emissive_out = float4(emissive.xyz * emissive_factor.xyz * material->emissive_factor[3], 1.0);

    // return
    return frag_out;
}