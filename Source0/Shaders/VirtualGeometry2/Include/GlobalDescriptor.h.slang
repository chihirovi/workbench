#pragma once
#include "ClusterRenderQueue.h.slang"

#define CLASSIFY_TILE_WIDTH (64)
#define CLASSIFY_THREAD_WIDTH (16)
#define CLASSIFY_MATERIAL_CHUNK_MAX (256)
#define MAX_MATERIAL (CLASSIFY_MATERIAL_CHUNK_MAX * 32)

[[vk_binding(0, 6)]]
RWStructuredBuffer<uint32_t> cluster_num_prefixsum;

[[vk_binding(1, 6)]]
RWStructuredBuffer<ClusterRenderQueue> cluster_render_queue;

[[vk_binding(2, 6)]]
[[vk_image_format("r64ui")]]
RWTexture2D<uint64_t> visiblity_buffer;

[[vk_binding(3, 6)]]
RWStructuredBuffer<uint32_t> mesh_draw_arg;

[[vk_binding(4, 6)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> output;

[[vk_binding(5, 6)]]
[[vk_image_format("r32ui")]]
RWTexture2D<uint32_t> material_id;

[[vk_binding(6, 6)]]
RWStructuredBuffer<uint32_t4> tile_indirect_draw_arg;

[[vk_binding(7, 6)]]
RWStructuredBuffer<uint32_t> tile_index;

[[vk_binding(8, 6)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> mat_tile_output;

[[vk_binding(9, 6)]]
RWStructuredBuffer<uint32_t> debug_buffer;

