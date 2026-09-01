#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

[shader("compute")]
[numthreads(32, 32, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    if(global_thread_id.x < 256 && global_thread_id.y==0){
        uint32_t mat_id = global_thread_id.x;
        tile_indirect_draw_arg[mat_id] = uint32_t4(6, 0, 0, mat_id);
    }
    
    uint2 screen_pos = global_thread_id.xy;
    if (screen_pos.x >= frame_render_data.render_target_width || screen_pos.y >= frame_render_data.render_target_height) return;
    
    if(screen_pos.x == 0 && screen_pos.y == 0){
        cluster_render_queue[0].head_num = 0;
        cluster_render_queue[0].tail_num = 0;
    }
    
    mat_tile_output[screen_pos] = float4(0.0);
    visiblity_buffer[screen_pos] = ((uint64_t(asuint(1.0f)) << 32) | 0xFFFFFFFF);
}