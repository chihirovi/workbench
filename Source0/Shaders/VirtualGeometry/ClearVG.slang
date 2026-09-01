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

    // freeze culling
    if(frame_render_data.editor_data[4] == 0){
        if(screen_pos.x == 0 && screen_pos.y == 0){

            cluster_render_queue[0].head_num = 0;
            cluster_render_queue[0].mesh_y = 1;
            cluster_render_queue[0].mesh_z = 1;

            cluster_render_queue[0].tail_num = 0;
            cluster_render_queue[0].soft_y = 1;
            cluster_render_queue[0].soft_z = 1;

            cluster_render_queue[0].head_num2 = 0;
            cluster_render_queue[0].mesh_y2 = 1;
            cluster_render_queue[0].mesh_z2 = 1;

            cluster_render_queue[0].tail_num2 = 0;
            cluster_render_queue[0].soft_y2 = 1;
            cluster_render_queue[0].soft_z2 = 1;

            cluster_render_queue[0].HW_fn_count = 0;     
            cluster_render_queue[0].SW_fn_count = 0;

            cluster_render_queue[0].sx = 0;
            cluster_render_queue[0].sy = 1;
            cluster_render_queue[0].sz = 1;
            cluster_render_queue[0].fn_count = 0;     
        }
    }
    
    mat_tile_output[screen_pos] = float4(0.0);
    visiblity_buffer[screen_pos] = ((uint64_t(asuint(1.0f)) << 33) | 0xFFFFFFFF);
}