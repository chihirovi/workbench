#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/VertexUtility.h.slang"

#include "./Include/payload.h.slang"

[[shader("mesh")]]
[[outputtopology("triangle")]]
[[numthreads(MAX_TRIANGLE, 1, 1)]]
void main_mesh(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID,
    in payload Task2MeshPayload payload,
    out indices uint3 triangles[MAX_TRIANGLE],
    out vertices Mesh2FragVertexPayload vertices[MAX_TRIANGLE*3],
    out primitives Mesh2FragPrimitivePayload primitives[MAX_TRIANGLE]){
    
    // set output status
    SetMeshOutputCounts(MAX_TRIANGLE*3, MAX_TRIANGLE);

    // early return
    if (global_thread_id.x >= payload.raw_triangle_num) {
        primitives[local_thread_id.x].is_culled = true;
        return;
    }

    // get index
    uint3 triangle_indices;
    triangle_indices[0] = ((uint32_t*)payload.raw_index_addr)[global_thread_id.x*3 + 0];
    triangle_indices[1] = ((uint32_t*)payload.raw_index_addr)[global_thread_id.x*3 + 1];
    triangle_indices[2] = ((uint32_t*)payload.raw_index_addr)[global_thread_id.x*3 + 2];

    // calc vertex
    float4 clip_poss[3];
    for(uint32_t i=0; i<3; i++){

        VertexType0 vertex = ((VertexType0*)payload.raw_vertex_addr)[triangle_indices[i]];

        // position
        float4 world_pos = mul(payload.model_matrix, float4(vertex.position, 1.0));
        float4 clip_pos = mul(frame_render_data.projection_view_matrix, world_pos);

        // normal
        // float3 normal = float3(float(vertex.normal[0]), float(vertex.normal[1]), float(vertex.normal[2]));
        float3 normal = normalize(decode_spherical_16(vertex.encode_normal));
        normal = normalize( mul(payload.reverse_transpose_3x3_model_matrix, normal));

        // tangent
        // float4 tangent = float4(float(vertex.tangent[0]), float(vertex.tangent[1]), float(vertex.tangent[2]), float(vertex.tangent[3]));
        float4 tangent;
        tangent.xyz = decode_spherical_16(vertex.encode_tangent[0]);
        tangent.w = (vertex.encode_tangent[1] == 1 ? 1.0 : -1.0);
        //tangent.xyz = normalize( mul(payload.reverse_transpose_3x3_model_matrix, tangent.xyz));
        tangent.xyz = normalize( mul(float32_t3x3(payload.model_matrix), tangent.xyz));

        // uv
        //float2 texcoord = float2(float(vertex.tex_coord_f32[0]), float(vertex.tex_coord_f32[1]));
        float2 texcoord = unpackHalf2x16ToFloat(vertex.packed_tex_coord);
        
        // color
        float4 color = float4( float(vertex.color[0]) / 255.0f, float(vertex.color[1]) / 255.0f, float(vertex.color[2]) / 255.0f, float(vertex.color[3]) / 255.0f);
        color.xyz = srgb_to_linear(color.xyz);
        color = color * payload.material_base_color_factor;

        //
        vertices[local_thread_id.x*3 + i].clip_position     = clip_pos;
        vertices[local_thread_id.x*3 + i].world_position    = world_pos;
        vertices[local_thread_id.x*3 + i].normal            = normal;
        vertices[local_thread_id.x*3 + i].tangent           = tangent;
        vertices[local_thread_id.x*3 + i].tex_coord         = texcoord;
        vertices[local_thread_id.x*3 + i].color             = color;
        
        clip_poss[i] = clip_pos;
    }

    // check backface
    bool is_backface = determinant(float32_t3x3(
        //vertices[local_thread_id.x*3 + 0u].clip_position.xyw, 
        //vertices[local_thread_id.x*3 + 1u].clip_position.xyw, 
        //vertices[local_thread_id.x*3 + 2u].clip_position.xyw
        clip_poss[0].xyw,
        clip_poss[1].xyw,
        clip_poss[2].xyw
    )) > 0;
    primitives[local_thread_id.x].is_front_face = !is_backface;

    // double side
    Material *p_material = &frame_render_data.material_table[payload.global_material_idx];
    bool is_double_side = p_material->is_double_side == 1 ? true : false;
    
    uint8_t3 tri;
    tri[0] = local_thread_id.x*3 + 0u;
    
    if(is_backface && is_double_side){
        tri[1] = local_thread_id.x*3 + 2u;
        tri[2] = local_thread_id.x*3 + 1u;
        primitives[local_thread_id.x].is_culled = false;

    } else if (is_backface && !is_double_side) {
        primitives[local_thread_id.x].is_culled = true;
        return;

    } else if (!is_backface) {
        tri[1] = local_thread_id.x*3 + 1u;
        tri[2] = local_thread_id.x*3 + 2u;
        primitives[local_thread_id.x].is_culled = false;
    }

    primitives[local_thread_id.x].global_material_id = payload.global_material_idx;
    triangles[local_thread_id.x] = tri;
    return;
}