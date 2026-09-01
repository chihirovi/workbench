#pragma once

#include "GlobalDescriptor.h.slang"
#include "../../Include/GlobalDescriptor.h.slang"

namespace VB{

uint32_t get_material_id(
    GPU_FrameRenderData frame_data,
    RWStructuredBuffer<ClusterRenderQueue, DefaultDataLayout> cluster_queue,
    RWTexture2D<uint64_t> vb,
    uint32_t2 screen_pos){

    uint64_t temp = vb[screen_pos];

    if(uint32_t(temp) != 0xFFFFFFFF){
        uint32_t render_queue_id = uint32_t(temp)>>7;
        uint32_t virtual_ref_id = uint32_t(cluster_queue[0].data[render_queue_id] >> 32);
        InstanceRefRange ref_range = frame_data.virtual_geometry_opaque_instance_ref_range;
        InstanceReference inst_ref = frame_data.instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
        Instance instance = frame_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
        return instance.material_id_base + uint32_t(instance.material_id);

    }else{
        return MAX_MATERIAL;
    }
}

float get_depth(
    GPU_FrameRenderData frame_data,
    RWTexture2D<uint64_t> vb,
    uint32_t2 screen_pos){

    if (any(screen_pos >= uint32_t2(frame_data.render_target_width, frame_data.render_target_height))) {
        return 1.0f; // Far Depth (Reversed-Z の場合は 0.0f に調整)
    }

    uint64_t temp = vb[screen_pos];
    uint32_t ud = uint32_t(temp >> 33);
    return asfloat(ud);
}

};