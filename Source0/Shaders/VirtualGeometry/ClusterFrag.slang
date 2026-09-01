#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

struct Mesh2FragVertexPayload {
    float4 clip_position    : SV_Position;
};

struct Mesh2FragPrimitivePayload {
    uint32_t render_queue_id : RENDER_QUEUE_ID;
    uint32_t triangle_id : TRIANGLE_ID;
};

[shader("fragment")]
void main_fragment(
    in Mesh2FragVertexPayload vertex_payload,
    in Mesh2FragPrimitivePayload primitive_payload){
    
    float depth = vertex_payload.clip_position.z;
    uint32_t render_queue_id = primitive_payload.render_queue_id;
    uint32_t triangle_id = primitive_payload.triangle_id;
    
    // float の符号bitを削って，SWRaster, HWRasterの識別に使っている
    uint32_t ud = asuint(depth);
    ud = ud << 1;
    uint64_t depth_render_queue_id = ((uint64_t(ud) << 32) | (render_queue_id << 7) | triangle_id);
    
    uint32_t2 pixel_posi = uint32_t2(vertex_payload.clip_position.xy);
    
    InterlockedMin(visiblity_buffer[pixel_posi], depth_render_queue_id);
}

