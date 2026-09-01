#pragma once
#include "../../Include/Instance.h.slang"
#include "../../Include/Material.h.slang"

// 64，32で比較したら，32が倍ぐらい早かった
// 64だと，L2キャッシュ使い過ぎ？
// ちなみに16でも+10%ぐらい遅くなっただけ，やっぱメインメモリ周りの遅延がやばそう
#define MAX_TRIANGLE uint32_t(32)

// group shaderdにはポインタは入れられないから，苦肉の策
struct Task2MeshPayload {
    uint32_t global_instance_idx;
    uint32_t global_material_idx;
    uint64_t raw_vertex_addr;
    uint64_t raw_index_addr;
    uint32_t raw_triangle_num;
    float32_t4x4 model_matrix;
    float32_t3x3 reverse_transpose_3x3_model_matrix;
    float4 material_base_color_factor;
};

groupshared Task2MeshPayload task_2_mesh_payload;

struct Mesh2FragVertexPayload{
    float4 clip_position    : SV_Position;
    float4 world_position   : POSITION;
    float3 normal           : NORMAL;
    float4 tangent          : TANGENT;
    float2 tex_coord        : TEXCOORD;
    float4 color            : COLOR;
};

struct Mesh2FragPrimitivePayload {
    bool is_culled          : SV_CullPrimitive;
    uint32_t global_material_id    : MATERIAL_ID;
    bool is_front_face      : IS_FRONTFACE;
};

struct Mesh2FragPrimitivePayloadFragment {
    uint32_t global_material_id    : MATERIAL_ID;
    bool is_front_face      : IS_FRONTFACE;
};

struct FragOut{
	float4 albedo_out           : SV_Target0; //rgba8unorm
	float4 world_position_out   : SV_Target1; //rgba32f
	float4 normal_occ_out       : SV_Target2; //rgba16f
	float4 metal_rough_out      : SV_Target3; //rg8unorm
	float4 emissive_out         : SV_Target4; //rgba8unorm
};