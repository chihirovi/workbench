#pragma once
#include <stdfloat>
#include <cstdint>
#include <stdfloat>
#include <type_traits>
#include <glm/glm.hpp>

namespace Irori::Render{

    //alignmentの関係で，フィールドの順番は変えないこと
    struct Instance{
        uint64_t        cluster_addr;
        uint64_t        cluster_vertex_addr;
        uint64_t        cluster_index_addr;        
        uint64_t        raw_vertex_addr;
        uint64_t        raw_index_addr;
        uint32_t        cluster_num;
        uint32_t        raw_vertex_num;
        uint32_t        raw_triangle_num;
        uint32_t        material_id_base;
        int32_t         material_id; //default materialを使う関係で，負になりうる
        glm::f32mat4x4  local_model_matrix;
        uint32_t        padding0;
        glm::f32mat3x3  local_reverse_transpose_3x3_model_matrix; // いらなくない？
        uint32_t        padding1;
        uint64_t        bvh8_node;
        uint64_t        reserved;
    }__attribute__((packed, aligned(1)));
    
    //168 = 8*21, spvで確認済み
    static_assert(sizeof(Instance) == 184, "Instance must be 128 bytes!");
} 