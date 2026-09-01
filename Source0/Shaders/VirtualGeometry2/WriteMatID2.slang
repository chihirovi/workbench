#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

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
        uint32_t virtual_ref_id = uint32_t(cluster_render_queue[0].data[render_queue_id] >> 32);
        InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
        InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
        Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
        material_id[screen_pos] = instance.material_id_base + uint32_t(instance.material_id);

    }else{
        material_id[screen_pos] = MAX_MATERIAL;
    }
}