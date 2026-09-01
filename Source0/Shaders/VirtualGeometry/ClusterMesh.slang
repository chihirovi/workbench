#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

#define CLUSTER_MAX_VERTEX      64
#define CLUSTER_MAX_TRIANGLE    126 //本当は126がよかったけど，meshoptimizerが%4==0でないといけないって

struct Mesh2FragVertexPayload {
    float4 clip_position    : SV_Position;
};

struct Mesh2FragPrimitivePayload {
    uint32_t render_queue_id : RENDER_QUEUE_ID;
    uint32_t triangle_id : TRIANGLE_ID;
};

groupshared Cluster cluster;
groupshared Instance instance;
groupshared InstanceReference inst_ref;

[[shader("mesh")]]
[[outputtopology("triangle")]]
[[numthreads(128, 1, 1)]]
void main_mesh(
    in uint3 global_thread_id   : SV_DispatchThreadID,
    in uint3 local_thread_id    : SV_GroupThreadID,
    in uint3 group_id           : SV_GroupID,
    out indices uint3 triangles[CLUSTER_MAX_TRIANGLE],
    out vertices Mesh2FragVertexPayload vertices[CLUSTER_MAX_VERTEX],
    out primitives Mesh2FragPrimitivePayload primitives[CLUSTER_MAX_TRIANGLE]){
    
    if(local_thread_id.x == 0){
        uint64_t temp = cluster_render_queue[0].data[group_id.x];
        uint32_t virtual_ref_id = uint32_t(temp >> 32);
        uint32_t cluster_id = uint32_t(temp & 0xFFFFFFFF);
        
        InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
        inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
        instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
    
        cluster = instance.cluster_addr[cluster_id];
    }
    GroupMemoryBarrierWithGroupSync();

    // set output status
    SetMeshOutputCounts(cluster.vertex_count, cluster.triangle_count);

    // vertex
    // 頂点の個数がポリゴンの個数を超える場合を想定してこの書き方をしてる
    // 今回の場合，スレッドの半数がこのループを処理してない
    for(int w = local_thread_id.x; w < cluster.vertex_count; w += 128){
        uint32_t vertex_idx = instance.cluster_vertex_addr[cluster.vertex_offset + w];

        float4 pos = float4(instance.raw_vertex_addr[vertex_idx].position, 1.0);
        float4 world_pos = mul(inst_ref.model_matrix, pos);
        float4 clip_pos = mul(frame_render_data.projection_view_matrix, world_pos);

        vertices[w].clip_position = clip_pos;
    }

    if (cluster.triangle_count <= local_thread_id.x) return;

    // triangle
    uint8_t3 triangle = uint8_t3(
        instance.cluster_index_addr[cluster.index_offset + local_thread_id.x * 3 + 0],
        instance.cluster_index_addr[cluster.index_offset + local_thread_id.x * 3 + 1],
        instance.cluster_index_addr[cluster.index_offset + local_thread_id.x * 3 + 2]
    );
    triangles[local_thread_id.x] = triangle;
    
    // primitive
    primitives[local_thread_id.x].render_queue_id   = group_id.x;
    primitives[local_thread_id.x].triangle_id       = local_thread_id.x;
}
