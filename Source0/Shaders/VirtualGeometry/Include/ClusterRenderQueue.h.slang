#pragma once

// (65536*16) == 1048576
#define MAX_RENDER_CLUSTER (65536*16)

struct ClusterRenderQueue {
    // 1st HW Raster Indirect Argument
    uint32_t    head_num;   // mesh shader indirect arg .x
    uint32_t    mesh_y;     // mesh shader indirect arg .y
    uint32_t    mesh_z;     // mesh shader indirect arg .z
    uint32_t    _padding0;
    // 1st SW Raster Indirect Argument
    uint32_t    tail_num;   // comp shader indirect arg .x
    uint32_t    soft_y;     // comp shader indirect arg .y
    uint32_t    soft_z;     // comp shader indirect arg .z
    uint32_t    _padding1;
    // 2st HW Raster Indirect Argument
    uint32_t    head_num2;   // mesh shader indirect arg .x
    uint32_t    mesh_y2;     // mesh shader indirect arg .y
    uint32_t    mesh_z2;     // mesh shader indirect arg .z
    uint32_t    _padding2;
    // 2st SW Raster Indirect Argument
    uint32_t    tail_num2;   // comp shader indirect arg .x
    uint32_t    soft_y2;     // comp shader indirect arg .y
    uint32_t    soft_z2;     // comp shader indirect arg .z
    uint32_t    _padding3;
    // false negative count
    uint32_t    HW_fn_count;     
    uint32_t    SW_fn_count;
    // false negative select arg
    uint32_t    sx;     
    uint32_t    sy;
    uint32_t    sz;
    uint32_t    fn_count;
    // data
    uint64_t    data[MAX_RENDER_CLUSTER];
    uint64_t    data_false_negative[MAX_RENDER_CLUSTER];
};