#include "Include/GlobalDEscriptor.h.slang"
#include "Include/VG2.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"
#include "../Include/FetchTexture.h.slang"

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
    
    float4 output = float4(1.0);
    
    if(frame_render_data.editor_data[15] == 314){
        output = fetch_texture_grad(
            frame_render_data,
            textures,
            samplers,
            0,
            0,
            0,
            float2(0.0), 
            float2(0.0), 
            float2(0.0), 
            float4(0.0)
        );
    }

    mat_tile_output[uint32_t2(vs_output.clip_position.xy)] = output;
}