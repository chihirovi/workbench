#include "Include/GlobalDEscriptor.h.slang"
#include "Include/Bary.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"
#include "../Include/FetchTexture.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/VertexUtility.h.slang"

struct VSOutput{
    float4 clip_position    : SV_Position;
    uint32_t mat_id         : MATERIAL_ID;
};

[[shader("vertex")]]
VSOutput main_vert(
    uint32_t vertex_id              : SV_VertexID,
    uint32_t instance_id            : SV_InstanceID,
    uint32_t first_instance         : SV_StartInstanceLocation) {
    
    
    uint32_t tile_num_x = (frame_render_data.render_target_width  + CLASSIFY_TILE_WIDTH -1)/CLASSIFY_TILE_WIDTH;
    uint32_t tile_num_y = (frame_render_data.render_target_height + CLASSIFY_TILE_WIDTH -1)/CLASSIFY_TILE_WIDTH;
    uint32_t tile_total_num = tile_num_x * tile_num_y;

    uint32_t mat_id = first_instance;
    uint32_t tile_offset = instance_id;
    uint32_t tile_idx = tile_index[tile_total_num * mat_id + tile_offset];

    uint tileX = tile_idx % tile_num_x; //15   30*19 =
    uint tileY = tile_idx / tile_num_x; //10
    
    tileX += (vertex_id == 2 || vertex_id == 3 || vertex_id == 5) ? 1 : 0;
    tileY += (vertex_id == 1 || vertex_id == 4 || vertex_id == 5) ? 1 : 0;
    float2 pos = float2(tileX * CLASSIFY_TILE_WIDTH, tileY * CLASSIFY_TILE_WIDTH);
    float2 screenPos = pos / float2(frame_render_data.render_target_width, frame_render_data.render_target_height);
    float2 clipPos = screenPos * float2(2, 2) - float2(1, 1);
    
    float depth = (float)mat_id / float(MAX_MATERIAL);
    
    VSOutput vs_output;
    vs_output.clip_position = float4(clipPos, depth, 1.0);
    vs_output.mat_id = mat_id;

    return vs_output;
}

[[shader("fragment")]]
[earlydepthstencil]
void main_fragment(in VSOutput vs_output) {

    uint32_t temp = uint32_t(visiblity_buffer[uint32_t2(vs_output.clip_position.xy)]);

    if (temp == 0xFFFFFFFF) return;
    
    uint32_t triangle_id = uint32_t(temp) & 0b01111111;
    uint32_t render_queue_id = uint32_t(temp) >> 7;
    uint32_t cluster_id = uint32_t(cluster_render_queue[0].data[render_queue_id]);
    uint32_t virtual_ref_id = uint32_t(cluster_render_queue[0].data[render_queue_id] >> 32);
    InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];

    Cluster cluster = instance.cluster_addr[cluster_id];
    uint32_t idx0 = instance.cluster_vertex_addr[cluster.vertex_offset + instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 0]];
    uint32_t idx1 = instance.cluster_vertex_addr[cluster.vertex_offset + instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 1]];
    uint32_t idx2 = instance.cluster_vertex_addr[cluster.vertex_offset + instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 2]];
    
    VertexType0 v0 = instance.raw_vertex_addr[idx0];
    VertexType0 v1 = instance.raw_vertex_addr[idx1];
    VertexType0 v2 = instance.raw_vertex_addr[idx2];
    
    float4 p0 = float4(v0.position, 1.0);
    float4 p1 = float4(v1.position, 1.0);
    float4 p2 = float4(v2.position, 1.0);

    float4 world_pos0 = mul(inst_ref.model_matrix, p0);
    float4 clip_pos0 = mul(frame_render_data.projection_view_matrix, world_pos0);

    float4 world_pos1 = mul(inst_ref.model_matrix, p1);
    float4 clip_pos1 = mul(frame_render_data.projection_view_matrix, world_pos1);

    float4 world_pos2 = mul(inst_ref.model_matrix, p2);
    float4 clip_pos2 = mul(frame_render_data.projection_view_matrix, world_pos2);

    float2 screenSize = float2(frame_render_data.render_target_width, frame_render_data.render_target_height);
    float2 screenPos = (vs_output.clip_position.xy + 0.5) / screenSize;
	float2 clipSpacePos = screenPos * float2(2, 2) - float2(1, 1);

    // get bary
    BarycentricDeriv bary = CalcFullBary(clip_pos0, clip_pos1, clip_pos2, clipSpacePos, screenSize);
    
    // uv
    float2 uv0 = unpackHalf2x16ToFloat(v0.packed_tex_coord);
    float2 uv1 = unpackHalf2x16ToFloat(v1.packed_tex_coord);
    float2 uv2 = unpackHalf2x16ToFloat(v2.packed_tex_coord);
    
    float2 uv, uv_dx, uv_dy;
    CalcDerivativeFloat2(bary, uv0, uv1, uv2, uv, uv_dx, uv_dy);
    
    // material
    Material *material = &frame_render_data.material_table[vs_output.mat_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;
    
    // normal
    float3 n0 = decode_spherical_16(v0.encode_normal);
    float3 n1 = decode_spherical_16(v1.encode_normal);
    float3 n2 = decode_spherical_16(v2.encode_normal);
    float3 n, n_dx, n_dy;
    CalcDerivativeFloat3(bary, n0, n1, n2, n, n_dx, n_dy);
    n = mul(inst_ref.reverse_transpose_3x3_model_matrix, n); // inverse のtypo
    
    // albedo
    float4 albedo = fetch_texture_grad(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material->texture_view_id_albedo,
        uv, 
        uv_dx,
        uv_dy,
        ALBEDO_TEX_DEFAULT_VALUE
    );
    albedo.xyz = srgb_to_linear(albedo.xyz);
    albedo = albedo * float4(material.base_color_factor[0], material.base_color_factor[1], material.base_color_factor[2], material.base_color_factor[3]);
    albedo.rgb *= max(0.01, abs(dot(n, frame_render_data.directional_light_dir.xyz)));
    albedo.rgb = linear_to_srgb(albedo.rgb);

    mat_tile_output[uint32_t2(vs_output.clip_position.xy)] = albedo;
}