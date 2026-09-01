#pragma once
#include <stdfloat>
#include <cstdint>
#include <stdfloat>

#define CLUSTER_MAX_VERTEX      64
#define CLUSTER_MAX_TRIANGLE    126 //本当は126がよかったけど，meshoptimizerが%4==0でないといけないって

namespace Irori::Render{

    //alignmentの関係で，フィールドの順番は変えないこと
    struct Cluster{
        uint32_t    vertex_offset;
        uint32_t    triangle_offset;
        uint8_t     vertex_count;
        uint8_t     triangle_count;
        uint8_t     lod_level;
        uint8_t     padding0;
        float       self_lod_bound_sphere[4];		//error計算用のlod bound sphereと，culling用の幾何学bound sphereは別々
        float       parent_lod_bound_sphere[4];	
        float       error_self;
        float       error_parent;
        float       self_aabb[6];		            //フラスタムカリングはこっち使う．
    }__attribute__((packed, aligned(1)));
    
    //76 = 4*19, spvで確認済み
    static_assert(sizeof(Cluster) == 76, "Cluster must be 76 bytes!");
}