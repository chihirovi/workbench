#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/VertexUtility.h.slang"
#include "../Include/FetchTexture.h.slang"

struct FragOut{
	float4 albedo_out           : SV_Target0; //rgba8unorm
	float4 world_position_out   : SV_Target1; //rgba32f
	float4 normal_occ_out       : SV_Target2; //rgba16f
	float4 metal_rough_out      : SV_Target3; //rg8unorm
	float4 emissive_out         : SV_Target4; //rgba8unorm
};

struct VSOutput{
    float4 clip_position    : SV_Position;
    float4 world_position   : POSITION;
    float3 normal           : NORMAL;
    float4 tangent          : TANGENT;
    float2 tex_coord        : TEXCOORD;
    float4 color            : COLOR;  
    uint32_t material_id    : MATERIAL;
};

[[shader("vertex")]]
VSOutput main_vert(
    uint32_t vertex_id              : SV_VertexID,
    uint32_t instance_ref_offset    : SV_StartInstanceLocation) {
        
    InstanceRefRange instance_ref_range = frame_render_data.common_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data.instance_reference_table[instance_ref_range.instance_ref_base + instance_ref_offset];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];

    uint32_t index = instance.raw_index_addr[vertex_id];
    VertexType0 vertex = instance.raw_vertex_addr[index];
    uint32_t material_id = instance.material_id_base + instance.material_id;
    Material *material = &frame_render_data.material_table[material_id];

    // material base color factor
    float4 material_bcf = float4(material.base_color_factor[0], material.base_color_factor[1], material.base_color_factor[2], material.base_color_factor[3]);
    material_bcf.xyz = srgb_to_linear(material_bcf.xyz);
    
    // position
    float4 world_pos = mul(inst_ref.model_matrix, float4(vertex.position, 1.0));
    float4 clip_pos = mul(frame_render_data.projection_view_matrix, world_pos);

    // normal
    // float3 normal = float3(float(vertex.normal[0]), float(vertex.normal[1]), float(vertex.normal[2]));
    float3 normal = decode_spherical_16(vertex.encode_normal);
    normal = normalize( mul(inst_ref.reverse_transpose_3x3_model_matrix, normal));

    // tangent
    // float4 tangent = float4(float(vertex.tangent[0]), float(vertex.tangent[1]), float(vertex.tangent[2]), float(vertex.tangent[3]));
    float4 tangent;
    tangent.xyz = decode_spherical_16(vertex.encode_tangent[0]);
    tangent.w = (vertex.encode_tangent[1] == 1 ? 1.0 : -1.0);
    tangent.xyz = normalize( mul(inst_ref.reverse_transpose_3x3_model_matrix, tangent.xyz));

    // uv
    //float2 texcoord = float2(float(vertex.tex_coord_f32[0]), float(vertex.tex_coord_f32[1]));
    float2 texcoord = unpackHalf2x16ToFloat(vertex.packed_tex_coord);
    
    // color
    float4 color = float4( float(vertex.color[0]) / 255.0f, float(vertex.color[1]) / 255.0f, float(vertex.color[2]) / 255.0f, float(vertex.color[3]) / 255.0f);
    color.xyz = srgb_to_linear(color.xyz);
    color = color * material_bcf;
    
    //
    VSOutput output;
    output.clip_position    = clip_pos;
    output.world_position   = world_pos;
    output.normal           = normal;
    output.tangent          = tangent;
    output.tex_coord        = texcoord;
    output.color            = color;
    output.material_id      = material_id;
    
    return output;
}

[shader("fragment")]
[earlydepthstencil]
FragOut main_fragment(in VSOutput vs_output, in bool is_front_face : SV_IsFrontFace) {
    
    FragOut frag_out;

    Material *material = &frame_render_data.material_table[vs_output.material_id];
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
        vs_output.tex_coord, 
        ALBEDO_TEX_DEFAULT_VALUE
    );
    albedo.xyz = srgb_to_linear(albedo.xyz);
    albedo = albedo * vs_output.color;
    frag_out.albedo_out = albedo;
    
    // world position
    frag_out.world_position_out = float4(vs_output.world_position.xyz, vs_output.clip_position.z);

    // normal occ
    float3 normal = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_normal,
        vs_output.tex_coord, 
        NORMAL_TEX_DEFAULT_VALUE
    ) .xyz;

    // ラスタライザでの法線補完では，長さは保たれない （線形補間なので)
    float3 N = normalize(vs_output.normal);
    float3 T = normalize(vs_output.tangent.xyz);
    float3 bitangent = normalize(cross(N, T))*vs_output.tangent.w;
    // tangent space->world space matrix
    float32_t3x3 its = transpose(float32_t3x3(T, bitangent, N));
    normal = normalize((normal * float3(material->normal_scale, material->normal_scale, 1) * 2.0 -1.0));
    normal = mul(its, normal);
    normal *= is_front_face ? 1.0 : -1.0;
    
    
    float occlusion = fetch_texture(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_occlusion, 
        vs_output.tex_coord,
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
        vs_output.tex_coord,
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
        vs_output.tex_coord,
        EMISSIVE_TEX_DEFAULT_VALUE
    );
    emissive.xyz = srgb_to_linear(emissive.xyz);
    float3 emissive_factor = float3(material->emissive_factor[0], material->emissive_factor[1], material->emissive_factor[2]);
    emissive_factor = srgb_to_linear(emissive_factor);
    frag_out.emissive_out = float4(emissive.xyz * emissive_factor.xyz * material->emissive_factor[3], 1.0);

    // return
    return frag_out;
}