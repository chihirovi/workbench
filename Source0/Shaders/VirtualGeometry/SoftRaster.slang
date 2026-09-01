#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

#define CLUSTER_MAX_VERTEX 64

// 共有メモリ: Cluster内の頂点（スクリーンスペース座標）をグループ内で共有
groupshared float4 g_screenPos[CLUSTER_MAX_VERTEX];

groupshared     InstanceReference   inst_ref;
groupshared     Instance    instance;        
groupshared     Cluster     cluster;

// FIX: model_matrix と projection_view_matrix を毎頂点2回掛けるのではなく、
// 1回だけ合成したMVP行列を共有メモリに置いて全スレッドで使い回す。
// (model_matrixはクラスタ全体で共通なので、頂点ごとに掛け直す意味が無い)
groupshared     float4x4    g_mvp;

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
    uint queue_index = MAX_RENDER_CLUSTER - 1 - group_id.x;

    // FIX: queue_index はスレッド内で不変かつピクセルにも依存しないので、
    // シェーダ冒頭で1回だけシフトしておく(元は各ピクセル書き込みの度に計算していた)
    uint32_t render_queue_id_shifted = queue_index << 7;

    if(local_id == 0){
        uint64_t temp = cluster_render_queue[0].data[queue_index];
        uint32_t virtual_ref_id = uint32_t(temp >> 32);
        uint32_t cluster_id     = uint32_t(temp & 0xFFFFFFFF);

        InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
        inst_ref = frame_render_data.instance_reference_table[ref_range.instance_ref_base + virtual_ref_id];
        instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
        cluster  = instance.cluster_addr[cluster_id];

        // FIX: MVP行列をここで1回だけ合成しておく
        g_mvp = mul(frame_render_data.projection_view_matrix, inst_ref.model_matrix);
    }
    GroupMemoryBarrierWithGroupSync();

    // FIX: ループ内で毎回 groupshared の cluster.xxx を読み直さないよう、
    // ループ不変な値をローカル変数へ退避しておく。
    uint32_t vertex_count   = cluster.vertex_count;
    uint32_t triangle_count = cluster.triangle_count;
    uint32_t vertex_offset  = cluster.vertex_offset;
    uint32_t index_offset   = cluster.index_offset;

    // -------------------------------------------------------------
    // 2. 頂点変換 & 共有メモリ（LDS）への格納
    // -------------------------------------------------------------
    float width  = (float)frame_render_data.render_target_width;
    float height = (float)frame_render_data.render_target_height;

    // スレッドで並列に頂点変換
    for(int vertex_id = local_id; vertex_id < vertex_count; vertex_id += THREAD_NUM){
        uint32_t vertex_idx = instance.cluster_vertex_addr[vertex_offset + vertex_id];
        float4 pos          = float4(instance.raw_vertex_addr[vertex_idx].position, 1.0);

        // FIX: model -> world -> clip の2段階乗算を、事前合成済みg_mvpによる
        // 1回の乗算に置き換え(world_posはこの後どこにも使われていないため不要)
        float4 clip_pos = mul(g_mvp, pos);

        // NDC -> スクリーンスペース座標 (Pixel Coordinates) への変換
        float3 ndc = clip_pos.xyz / clip_pos.w;
        float2 pixel_pos = float2(
            (ndc.x * 0.5 + 0.5) * width,
            (ndc.y * 0.5 + 0.5) * height
        );

        // g_screenPos.z には 0~1 深度（または clip_pos.z/w）、w には 1/clip_pos.w を保持
        g_screenPos[vertex_id] = float4(pixel_pos, ndc.z, 1.0 / clip_pos.w);
    }

    GroupMemoryBarrierWithGroupSync();

    // -------------------------------------------------------------
    // 3. 三角形単位のソフトウェアラスタライズ (1スレッド = 1ポリゴン)
    // -------------------------------------------------------------
    for(int triangle_id = local_id; triangle_id < triangle_count; triangle_id += THREAD_NUM){

        // インデックスの取得
        uint32_t idx0 = instance.cluster_index_addr[index_offset + triangle_id * 3 + 0];
        uint32_t idx1 = instance.cluster_index_addr[index_offset + triangle_id * 3 + 1];
        uint32_t idx2 = instance.cluster_index_addr[index_offset + triangle_id * 3 + 2];

        float4 v0 = g_screenPos[idx0];
        float4 v1 = g_screenPos[idx1];
        float4 v2 = g_screenPos[idx2];

        // FIX: return だとこのスレッドが担当する残りの三角形まで
        // 全て処理されなくなってしまう(THREAD_NUMを超えるtriangle_countの場合)。
        // このループの残りをスキップするだけの continue に修正。
        if (v0.w < 0.0f || v1.w < 0.0f || v2.w < 0.0f) continue;
        if (v0.z < 1e-5f || v1.z < 1e-5f || v2.z < 1e-5f) continue;

        // 三角形の面積（Oriented Area）
        float area = EdgeFunction(v0.xy, v1.xy, v2.xy);
        if (area == 0.0) continue; // 縮退三角形スキップ
        if(area < 0.0){
            float4 temp = v1;
            v1 = v2;
            v2 = temp;
            area *= -1.0f;
        }

        // FIX: ピクセル毎に3回割り算する代わりに、三角形につき1回だけ逆数を計算
        float inv_area = 1.0f / area;

        // 2Dバウンディングボックス（AABB）の算出
        float2 min_p = min(v0.xy, min(v1.xy, v2.xy));
        float2 max_p = max(v0.xy, max(v1.xy, v2.xy));

        // 画面解像度内にクランプ（整数字数ピクセル化）
        int2 min_pixel = max(int2(0, 0), int2(floor(min_p)));
        int2 max_pixel = min(int2(width - 1, height - 1), int2(ceil(max_p)));

        // 三角形に紐づく、ピクセル座標に依存しない部分を先に計算しておく
        uint32_t render_queue_and_tri = render_queue_id_shifted | uint32_t(triangle_id);

        // ----------------------------------------------------------------
        // FIX: エッジ関数のインクリメンタル評価
        //
        // EdgeFunction(a,b,c) は c(ピクセル座標)に対してアフィン関数
        // (w(x,y) = A*x + B*y + C)なので、毎ピクセル フルで計算し直す代わりに、
        // 開始点での値だけ計算し、以降はX方向/Y方向の一定増分を足すだけにする。
        // 大きい三角形ほど効果が大きい、ソフトラスタライザの定番最適化。
        // ----------------------------------------------------------------
        float A0 = v2.y - v1.y;
        float B0 = v1.x - v2.x;
        float A1 = v0.y - v2.y;
        float B1 = v2.x - v0.x;
        float A2 = v1.y - v0.y;
        float B2 = v0.x - v1.x;

        float2 p_start = float2(min_pixel) + 0.5; // 開始ピクセルの中心
        float w0_row = EdgeFunction(v1.xy, v2.xy, p_start);
        float w1_row = EdgeFunction(v2.xy, v0.xy, p_start);
        float w2_row = EdgeFunction(v0.xy, v1.xy, p_start);

        // Bounding Box内のピクセルを走査（ソフトラスタライズ）
        for (int y = min_pixel.y; y <= max_pixel.y; ++y){

            float w0 = w0_row;
            float w1 = w1_row;
            float w2 = w2_row;

            for (int x = min_pixel.x; x <= max_pixel.x; ++x){

                // ピクセルが三角形の内側にあるか確認
                if (w0 >= 0.0 && w1 >= 0.0 && w2 >= 0.0){
                    // 重心座標（Barycentric Coordinates）正規化
                    float bw0 = w0 * inv_area;
                    float bw1 = w1 * inv_area;
                    float bw2 = w2 * inv_area;

                    // 遠近法補正（Perspective-Correct Interpolation）による深度算出
                    float depth = bw0 * v0.z + bw1 * v1.z + bw2 * v2.z;

                    // フラグメントシェーダーと同じPacked Depth & Payload (64bit) の作成
                    // float の符号bitを削って，SWRaster, HWRasterの識別に使っている
                    uint32_t ud = asuint(depth);
                    ud = ud << 1 | uint32_t(1);
                    uint64_t depth_render_queue_id = (uint64_t(ud) << 32) | render_queue_and_tri;

                    // Visibility Bufferへ64bit Atomic Min書き込み
                    uint2 pixel_posi = uint2(x, y);
                    InterlockedMin(visiblity_buffer[pixel_posi], depth_render_queue_id);
                }

                w0 += A0;
                w1 += A1;
                w2 += A2;
            }

            w0_row += B0;
            w1_row += B1;
            w2_row += B2;
        }
    }
}