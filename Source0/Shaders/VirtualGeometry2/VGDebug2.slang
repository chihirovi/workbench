#include "Include/GlobalDEscriptor.h.slang"
#include "Include/VG2.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

static const float3 ColorTable[16] = {
    float3(1.0, 0.0, 0.0),   // 赤
    float3(0.0, 1.0, 0.0),   // 緑
    float3(0.0, 0.0, 1.0),   // 青
    float3(1.0, 1.0, 0.0),   // 黄
    float3(1.0, 0.0, 1.0),   // マゼンタ
    float3(0.0, 1.0, 1.0),   // シアン
    float3(1.0, 0.5, 0.0),   // オレンジ
    float3(0.5, 0.0, 1.0),   // 紫
    float3(0.0, 0.5, 1.0),   // スカイブルー
    float3(0.0, 1.0, 0.5),   // 黄緑
    float3(1.0, 0.0, 0.5),   // ピンク
    float3(0.5, 1.0, 0.0),   // ライム
    float3(0.3, 0.3, 0.3),   // グレー
    float3(0.7, 0.7, 0.7),   // 明るいグレー
    float3(0.5, 0.25, 0.1),  // ブラウン
    float3(0.1, 0.7, 0.9)    // アクア
};

static const float3 ColorTable2[8] = {
    float3(1.0, 0.0, 0.0),   // 赤
    float3(1.0, 0.5, 0.0),   // オレンジ
    float3(1.0, 1.0, 0.0),   // 黄色
    float3(0.5, 1.0, 0.0),   // 黄緑〜緑黄
    float3(0.0, 1.0, 0.5),   // 黄緑
    float3(0.0, 0.5, 1.0),   // 水色（シアン寄り）
    float3(0.0, 0.0, 1.0),   // 青
    float3(0.0, 0.0, 0.5),   // 濃い青
};

[shader("compute")]
[numthreads(32, 32, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    uint2 screen_pos = global_thread_id.xy;
    if (screen_pos.x >= frame_render_data.render_target_width || screen_pos.y >= frame_render_data.render_target_height) return;
    
    uint64_t temp = visiblity_buffer[screen_pos];
    
    if(uint32_t(temp) != 0xFFFFFFFF){
        uint32_t render_queue_id = uint32_t(temp)>>7;
        uint32_t triangle_id = uint32_t(temp) & 0b01111111;
        
        uint32_t mode = frame_render_data.editor_data[0];
        if (mode == 0) {
            output[screen_pos] = float4(ColorTable[triangle_id%16], 1.0);

        }else if(mode == 1){
            uint32_t cluster_id = uint32_t(cluster_render_queue[0].data[render_queue_id]);
            output[screen_pos] = float4(ColorTable[cluster_id % 16], 1.0)*mat_tile_output[screen_pos]*2.0;

        } else if (mode == 2) {
            
            output[screen_pos] = material_id[screen_pos] == MAX_MATERIAL ? float4(0.0) : float4(ColorTable[material_id[screen_pos]%16], 1.0) ;

        }else if(mode == 3){
            
            output[screen_pos] = mat_tile_output[screen_pos];

        }else if(mode == 4){
            uint32_t cluster_id = uint32_t(cluster_render_queue[0].data[render_queue_id]);
            uint32_t virtual_ref_id = uint32_t(cluster_render_queue[0].data[render_queue_id] >> 32);
            InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
            InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
            Instance instance = frame_render_data .instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
            ClusterVG2 cluster = ((ClusterVG2*)instance.cluster_addr)[cluster_id];
            output[screen_pos] = float4(ColorTable[cluster.lod%16], 1.0);
            //output[screen_pos] = float4(ColorTable2[cluster.lod%8], 1.0);
        }
    }else{
        output[screen_pos] = float4(0.0, 0.0, 0.0, 0.0);
    }
}