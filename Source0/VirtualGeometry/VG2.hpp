#pragma once
#include <cstdint>

namespace Irori::VirtualGeometry2{
    struct ClusterVG2 {
        uint32_t index_offset;
        uint32_t vertex_offset;
        float self_err_bounds[4];
        float self_err;
        float parent_err_bounds[4];
        float parent_err;
        float geometry_bounds[4];
        uint8_t triangle_count;
        uint8_t vertex_count;
        uint8_t lod;
        uint8_t _padding0;
    }; // 68 byte
       
    struct BVH8NodeVG2 {
        uint32_t children[8];
        float err_bounds[4]; // すべての子供のparent_err_boundsのmerged error
        float geometry_bounds[4];
        uint8_t valid_children_count;
        uint8_t has_cluster; // 1 == true, 0 == false
        uint8_t _padding0;
        uint8_t _padding1;
    }; // 68 byte
    
    struct VG2Header {
        uint32_t gltf_mesh_idx;
        uint32_t gltf_primitive_idx;
        uint32_t cluster_byte_offset;
        uint32_t cluster_count;
        uint32_t bvh8_node_byte_offset;
        uint32_t bvh8_node_count;
        uint32_t vertex_byte_offset;
        uint32_t vertex_count;
        uint32_t index_byte_offset;
        uint32_t index_count;
    };

    struct VG2File{
        VG2Header header;
        std::vector<ClusterVG2> clusters;
        std::vector<BVH8NodeVG2> bvh8_nodes;
        std::vector<float> vertex_positions;
        std::vector<uint8_t> indices;
    };
    
    VG2File load_vg2_file(const char *filename);
}