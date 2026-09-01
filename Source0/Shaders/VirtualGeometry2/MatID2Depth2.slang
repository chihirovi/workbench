#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

struct FragOut{
	float depth             : SV_Depth;
};

struct VSOutput{
    float4 clip_position    : SV_Position;
};

static const float2 positions[6] = {
    float2(-1.0f, -1.0f),   //左下
    float2(-1.0f,  1.0f),   //左上
    float2( 1.0f,  1.0f),   //右上
    float2(-1.0f, -1.0f),   //左下
    float2( 1.0f,  1.0f),   //右上
    float2( 1.0f, -1.0f)    //右下
};

[[shader("vertex")]]
VSOutput main_vert(
    uint32_t vertex_id              : SV_VertexID,
    uint32_t instance_ref_offset    : SV_StartInstanceLocation) {
    
    VSOutput vs_output;
    vs_output.clip_position = float4(positions[vertex_id], 1.0, 1.0);

    return vs_output;
}

[shader("fragment")]
FragOut main_fragment(in VSOutput vs_output) {

    uint32_t2 pixel_posi = uint32_t2(vs_output.clip_position.xy);
    FragOut frag_out;
    
    frag_out.depth = float(material_id[pixel_posi])/float(MAX_MATERIAL);
    
    // return
    return frag_out;
}