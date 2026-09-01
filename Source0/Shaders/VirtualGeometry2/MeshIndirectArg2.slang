#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

[shader("compute")]
[numthreads(1, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    mesh_draw_arg[0] = cluster_render_queue[0].head_num;
    mesh_draw_arg[1] = 1;
    mesh_draw_arg[2] = 1;
}