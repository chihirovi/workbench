#pragma once

#define MAX_RENDER_CLUSTER (65536*8)

struct ClusterRenderQueue {
    uint32_t    head_num;
    uint32_t    tail_num;
    uint64_t    data[MAX_RENDER_CLUSTER];
};