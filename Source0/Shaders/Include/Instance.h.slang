#pragma once
#include "Cluster.h.slang"
#include "VertexType.h.slang"

// alignmentの関係で，フィールドの順番は変えないこと
struct Instance{
    Cluster*        cluster_addr;
    uint32_t*       cluster_vertex_addr;
    uint8_t*        cluster_index_addr;        
    VertexType0*    raw_vertex_addr;
    uint32_t*       raw_index_addr;
    uint32_t        cluster_num;
    uint32_t        raw_vertex_num;
    uint32_t        raw_triangle_num;
    uint32_t        material_id_base;
    int32_t         material_id; //default materialを使う関係で，負になりうる
    float32_t4x4    model_matrix;
    uint32_t        padding0;
    float32_t3x3    local_reverse_transpose_3x3_model_matrix; // いらなくない？
    uint32_t        padding1;
    uint64_t        bvh8_node;
    uint64_t        reserved;
};
