#pragma once

#include "../../Include/VertexType.h.slang"
#include "../../Include/GPU_FrameRenderData.h.slang"
#include "../../Include/Rt.h.slang"
#include "Types.h.slang"

uint8_t calc_lod(float3 camera_center, float3 probe_center, float lod_offset) {
    float dist = length(camera_center - probe_center);
    uint8_t lod = uint8_t(dist / lod_offset);
    lod = clamp(lod, uint8_t(0), uint8_t(4));
    return lod;
}

void get_as_lod_vertex_index_addr(
    uint8_t lod,
    GPU_FrameRenderData frame_render_data_,
    RtPayload rt_payload,
    out VertexType0* vertex_addr, 
    out uint32_t* index_addr) {
    
    if(lod == 0){
        Instance instance = frame_render_data_.instance_table[rt_payload.render_instance_id_base + rt_payload.render_instance_id];
        vertex_addr = instance.raw_vertex_addr;
        index_addr = instance.raw_index_addr;

    } else {
        uint32_t as_stride = (uint32_t)frame_render_data_.rt_lod1234_vert_idx_addr[0];
        uint32_t offset = 1 + ((lod-1) * as_stride + rt_payload.render_instance_id_base + rt_payload.render_instance_id) * 2;
        vertex_addr = (VertexType0*) frame_render_data_.rt_lod1234_vert_idx_addr[offset + 0];
        index_addr = (uint32_t*) frame_render_data_.rt_lod1234_vert_idx_addr[offset + 1];
    }
}