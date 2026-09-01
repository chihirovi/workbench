#pragma once

struct ClusterVG2 {
    uint32_t index_offset;
    uint32_t vertex_offset;
    float32_t self_err_bounds[4];
    float32_t self_err;
    float32_t parent_err_bounds[4];
    float32_t parent_err;
    float32_t geometry_bounds[4];
    uint8_t triangle_count;
    uint8_t vertex_count;
    uint8_t lod;
    uint8_t _padding0;
}; // 60 byte
   
struct BVH8NodeVG2 {
    uint32_t children[8];
    float32_t err_bounds[4]; // すべての子供のparent_err_boundsのmerged error
    float32_t geometry_bounds[4];
    uint8_t valid_children_count;
    uint8_t has_cluster; // 1 == true, 0 == false
    uint8_t _padding0;
    uint8_t _padding1;
}; // 68 byte