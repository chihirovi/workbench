#include "Include/GlobalDescriptor.h.slang"
#include "Include/Culling.h.slang"
#include "../Include/GlobalDescriptor.h.slang"

/// グループ全体で合算するための共有メモリ
groupshared uint g_groupActiveCountHWRasterFN;
groupshared uint g_groupBaseIndexHWRasterFN;
groupshared uint g_groupActiveCountSWRasterFN;
groupshared uint g_groupBaseIndexSWRasterFN;

#define CTA_NUM 128

[shader("compute")]
[numthreads(CTA_NUM, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {

    uint32_t thread_id = global_thread_id.x;

    // check thread range
    if(frame_render_data.editor_data[4] == 1) return;

    bool is_active = true;
    if(cluster_render_queue[0].fn_count <= thread_id){
        is_active = false;
        thread_id = cluster_render_queue[0].fn_count - 1;
    }

    // get cluster info
    uint64_t temp;
    bool is_hard_raster = false;
    if(thread_id < cluster_render_queue[0].HW_fn_count){
        temp = cluster_render_queue[0].data_false_negative[thread_id];
        is_hard_raster = true;
    }else{
        uint32_t read_idx = MAX_RENDER_CLUSTER - 1 - (thread_id - cluster_render_queue[0].HW_fn_count);
        temp = cluster_render_queue[0].data_false_negative[read_idx];
    }

    bool is_soft_raster = !is_hard_raster;

    uint32_t virtual_ref_idx = uint32_t(temp >> 32);
    uint32_t cluster_id     = uint32_t(temp & 0xFFFFFFFF);

    //
    InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_idx];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
    Cluster cluster = instance.cluster_addr[cluster_id];

    // ==========================
    // culling
    // ==========================
    float3 aabb_min = float3(cluster.self_aabb[0], cluster.self_aabb[1], cluster.self_aabb[2]);
    float3 aabb_max = float3(cluster.self_aabb[3], cluster.self_aabb[4], cluster.self_aabb[5]);
    float3 bs_center = (aabb_min + aabb_max)/2.0;
    float bs_r = length(aabb_max - bs_center);

    // hiz
    ProjectionBoundingSphereInfo bs_info = projection_bounding_sphere(
        float4(bs_center, bs_r),
        mul(frame_render_data.projection_view_matrix, inst_ref.model_matrix),
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        0.00f
    );

    // micro cluster culling
    bool is_micro_cluster = CoversPixelCenter(bs_info);
    is_active &= is_micro_cluster;

    //bool passed_hiz = hiz_occlusion_culling2(
    bool passed_hiz = hiz_occlusion_culling3(
        bs_info,
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        occlusion_hiz
    );

    // recode
    // -------------------------------------------------------------
    // 1. Wave内の集計
    // -------------------------------------------------------------
    uint waveActiveCountHWRasterFN = WaveActiveCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveLocalIndexHWRasterFN  = WavePrefixCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveActiveCountSWRasterFN = WaveActiveCountBits(is_active & passed_hiz & is_soft_raster);
    uint waveLocalIndexSWRasterFN  = WavePrefixCountBits(is_active & passed_hiz & is_soft_raster);

    // 各Waveの先頭スレッド（Leader）が、グループ共有メモリにWaveごとのカウントを足し込む
    if (local_thread_id.x == 0)
    {
        g_groupActiveCountHWRasterFN = 0;
        g_groupActiveCountSWRasterFN = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint waveBaseInGroupHWRasterFN = 0;
    uint waveBaseInGroupSWRasterFN = 0;

    if (WaveIsFirstLane())
    {
        // 共有メモリに対するAtomic加算（高速・Wave間で集計）
        InterlockedAdd(g_groupActiveCountHWRasterFN, waveActiveCountHWRasterFN, waveBaseInGroupHWRasterFN);
        InterlockedAdd(g_groupActiveCountSWRasterFN, waveActiveCountSWRasterFN, waveBaseInGroupSWRasterFN);
    }

    // 全Waveによる共有メモリ(g_groupActiveCount)への加算完了を待つ
    GroupMemoryBarrierWithGroupSync();

    // Wave内のLeaderが得た「グループ内でのオフセット」をWave内の全スレッドへ共有
    waveBaseInGroupHWRasterFN = WaveReadLaneFirst(waveBaseInGroupHWRasterFN);
    waveBaseInGroupSWRasterFN = WaveReadLaneFirst(waveBaseInGroupSWRasterFN);

    // -------------------------------------------------------------
    // 2. グローバルAtomic Add（Thread 0 のみ実行）
    // -------------------------------------------------------------
    if (local_thread_id.x == 0)
    {
        // グループ全体の合計数をグローバルキューへ1回だけAtomic Add
        InterlockedAdd(cluster_render_queue[0].head_num2, g_groupActiveCountHWRasterFN, g_groupBaseIndexHWRasterFN);
        InterlockedAdd(cluster_render_queue[0].tail_num2, g_groupActiveCountSWRasterFN, g_groupBaseIndexSWRasterFN);
    }
    GroupMemoryBarrierWithGroupSync(); // グローバルIndexの取得完了待ち

    // -------------------------------------------------------------
    // 3. 書き込み位置の計算と書き込み
    // -------------------------------------------------------------
    // グローバル基準値 + グループ内Waveオフセット + Wave内スレッドオフセット
    if(is_active & passed_hiz & is_hard_raster){
        uint writeIndex = g_groupBaseIndexHWRasterFN + waveBaseInGroupHWRasterFN + waveLocalIndexHWRasterFN;
        writeIndex += cluster_render_queue[0].head_num;
        if(writeIndex < MAX_RENDER_CLUSTER){
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }

    if(is_active & passed_hiz & is_soft_raster){
        uint writeIndex = g_groupBaseIndexSWRasterFN + waveBaseInGroupSWRasterFN + waveLocalIndexSWRasterFN;
        writeIndex += cluster_render_queue[0].tail_num;
        if(writeIndex < MAX_RENDER_CLUSTER){
            writeIndex = MAX_RENDER_CLUSTER - 1 - writeIndex;
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }
}