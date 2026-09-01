#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

#define CLUSTER_MAX_VERTEX 64
#define CLUSTER_MAX_TRIANGLE 126 //本当は126がよかったけど，meshoptimizerが%4==0でないといけないって

// 共有メモリ: Cluster内の頂点（スクリーンスペース座標）をグループ内で共有
groupshared float4 g_screenPos[CLUSTER_MAX_VERTEX];

groupshared     InstanceReference   inst_ref;
groupshared     Instance    instance;        
groupshared     Cluster     cluster;

// 2Dにおける三角形の外積（エッジ関数）
float EdgeFunction(float2 a, float2 b, float2 c){
    return (c.x - a.x) * (b.y - a.y) - (c.y - a.y) * (b.x - a.x);
}

#define THREAD_NUM 128
[shader("compute")]
[numthreads(THREAD_NUM, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id  : SV_GroupThreadID,
    in uint3 group_id         : SV_GroupID){

    uint local_id = local_thread_id.x;

    // -------------------------------------------------------------
    // 1. キューから対象Cluster情報の読み出し (MAX_RENDER_CLUSTERの末尾から)
    // -------------------------------------------------------------
    uint queue_index = MAX_RENDER_CLUSTER - 1 - (cluster_render_queue[0].tail_num + group_id.x);

    if(local_id == 0){
        uint64_t temp = cluster_render_queue[0].data[queue_index];
        uint32_t virtual_ref_id = uint32_t(temp >> 32);
        uint32_t cluster_id     = uint32_t(temp & 0xFFFFFFFF);

        InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
        inst_ref = frame_render_data.instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
        instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
        cluster  = instance.cluster_addr[cluster_id];
    }
    GroupMemoryBarrierWithGroupSync();

    // -------------------------------------------------------------
    // 2. 頂点変換 & 共有メモリ（LDS）への格納
    // -------------------------------------------------------------
    float width  = (float)frame_render_data.render_target_width;
    float height = (float)frame_render_data.render_target_height;

    // スレッドで並列に頂点変換
    for(int vertex_id = local_id; vertex_id < cluster.vertex_count; vertex_id += THREAD_NUM){
        uint32_t vertex_idx = instance.cluster_vertex_addr[cluster.vertex_offset + vertex_id];
        float4 pos          = float4(instance.raw_vertex_addr[vertex_idx].position, 1.0);
        float4 world_pos    = mul(inst_ref.model_matrix, pos);
        float4 clip_pos     = mul(frame_render_data.projection_view_matrix, world_pos);

        // NDC -> スクリーンスペース座標 (Pixel Coordinates) への変換
        float3 ndc = clip_pos.xyz / clip_pos.w;
        float2 pixel_pos = float2(
            (ndc.x * 0.5 + 0.5) * width,
            (ndc.y * 0.5 + 0.5) * height
        );

        // g_screenPos.z には 0~1 深度（または clip_pos.z/w）、w には 1/clip_pos.w を保持
        g_screenPos[vertex_id] = float4(pixel_pos, ndc.z, 1.0 / clip_pos.w);
    }

    // 全頂点のスクリーン座標変換完了を待つ
    GroupMemoryBarrierWithGroupSync();

    // -------------------------------------------------------------
    // 3. 三角形単位のソフトウェアラスタライズ (1スレッド = 1ポリゴン)
    // -------------------------------------------------------------
    for(int triangle_id = local_id; triangle_id < cluster.triangle_count; triangle_id += THREAD_NUM){

        // インデックスの取得
        uint32_t idx0 = instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 0];
        uint32_t idx1 = instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 1];
        uint32_t idx2 = instance.cluster_index_addr[cluster.index_offset + triangle_id * 3 + 2];

        float4 v0 = g_screenPos[idx0];
        float4 v1 = g_screenPos[idx1];
        float4 v2 = g_screenPos[idx2];

        // カリング（画面外/ニアプレーン裏チェック）
        if (v0.w < 0.0f || v1.w < 0.0f || v2.w < 0.0f) return;
        if (v0.z < 1e-5f || v1.z < 1e-5f || v2.z < 1e-5f) return;

        // 三角形の面積（Oriented Area）
        float area = EdgeFunction(v0.xy, v1.xy, v2.xy);
        if (area == 0.0) return; // バックフェイスカリング（裏面スキップ）
        if(area < 0.0){
            float4 temp = v1;
            v1 = v2;
            v2 = temp;
            area *= -1.0f;
        }

        // 2Dバウンディングボックス（AABB）の算出
        float2 min_p = min(v0.xy, min(v1.xy, v2.xy));
        float2 max_p = max(v0.xy, max(v1.xy, v2.xy));

        // 画面解像度内にクランプ（整数字数ピクセル化）
        int2 min_pixel = max(int2(0, 0), int2(floor(min_p)));
        int2 max_pixel = min(int2(width - 1, height - 1), int2(ceil(max_p)));

        // Bounding Box内のピクセルを走査（ソフトラスタライズ）
        for (int y = min_pixel.y; y <= max_pixel.y; ++y){
            for (int x = min_pixel.x; x <= max_pixel.x; ++x){

                float2 p = float2(x, y) + 0.5; // ピクセル中心

                // エッジ関数（重心座標系判定）
                float w0 = EdgeFunction(v1.xy, v2.xy, p);
                float w1 = EdgeFunction(v2.xy, v0.xy, p);
                float w2 = EdgeFunction(v0.xy, v1.xy, p);

                // ピクセルが三角形の内側にあるか確認
                if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0){
                    // 重心座標（Barycentric Coordinates）正規化
                    w0 /= area;
                    w1 /= area;
                    w2 /= area;

                    // 遠近法補正（Perspective-Correct Interpolation）による深度算出
                    float depth = w0 * v0.z + w1 * v1.z + w2 * v2.z;

                    // フラグメントシェーダーと同じPacked Depth & Payload (64bit) の作成
                    uint32_t render_queue_id        = queue_index;
                    //uint32_t triangle_id            = local_id;

                    // float の符号bitを削って，SWRaster, HWRasterの識別に使っている
                    uint32_t ud = asuint(depth);
                    ud = ud << 1 | uint32_t(1);
                    uint64_t depth_render_queue_id  = ((uint64_t(ud) << 32) | (render_queue_id << 7) | triangle_id);

                    // Visibility Bufferへ64bit Atomic Min書き込み
                    uint2 pixel_posi = uint2(x, y);
                    InterlockedMin(visiblity_buffer[pixel_posi], depth_render_queue_id);
                }
            }
        }
    }
}