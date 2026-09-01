#include "Include/GlobalDescriptor.h.slang"
#include "../Include/GlobalDescriptor.h.slang"

/*
// グループ共有メモリ: 16x16 のダウンサンプル計算用 (256スレッド)
groupshared float gs_depth_samples[16][16];
groupshared uint32_t gs_spd_counter;

//------------------------------------------------------------------------------
// ヘルパー関数: 2x2 の Depth サンプルから最小値（最手前のDepth）を抽出
//------------------------------------------------------------------------------
float ReduceMax4(float d0, float d1, float d2, float d3) {
    return max(max(d0, d1), max(d2, d3));
}

#define IMAGE_SIZE uint32_t2(frame_render_data.render_target_width, frame_render_data.render_target_height)

// 64bit Visibility Buffer から Depth (float) を復元
float fetch_depth_from_visiblity_buffer(uint2 pos) {
    // 画面外アクセスの安全対策
    if (any(pos >= IMAGE_SIZE)) {
        return 1.0f; // Far Depth (Reversed-Z の場合は 0.0f に調整)
    }
    uint64_t temp = visiblity_buffer[pos];
    uint32_t ud = uint32_t(temp >> 33);
    return asfloat(ud);
}

// ============================================================================
// 2048 x 2048
//
// mip 0 : 2048 x 2048
// mip 1 : 1024 x 1024
// mip 2 :  512 x  512
// mip 3 :  256 x  256
// mip 4 :  128 x  128
// mip 5 :   64 x   64
// mip 6 :   32 x   32
// mip 7 :   16 x   16
// mip 8 :    8 x    8
// mip 9 :    4 x    4
// mip10 :    2 x    2
// mip11 :    1 x    1
// ============================================================================

// ============================================================================
// Mip size
//
// ceil(size / 2^mip)
// ============================================================================

uint2 GetMipSize(uint mip){
    uint2 size = IMAGE_SIZE;
    for (uint i = 0; i < mip; ++i){
        size = max((size + 1u) >> 1u, uint2(1, 1));
    }
    return size;
}

bool InBounds(uint2 p, uint2 size){
    return all(p < size);
}
// ============================================================================
// Mip load
// ============================================================================

float FetchMipDepth(uint2 pos, uint mip){
    uint2 size = GetMipSize(mip);

    if (!InBounds(pos, size)) return 1.0f;

    return occlusion_hiz[mip][pos];
}

float FetchMip4(uint2 pos){
    uint2 size = GetMipSize(4);
    if (any(pos >= size)) return 1.0f;
    return occlusion_hiz_mip4[pos];
}

// ============================================================================
// Main
// ============================================================================

#define THREAD_NUM 256
[shader("compute")]
[numthreads(THREAD_NUM, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id  : SV_GroupThreadID,
    in uint3 group_id         : SV_GroupID){

    uint thread_idx = local_thread_id.x;

    uint2 thread_2d = uint2(thread_idx & 15u, thread_idx >> 4u);

    // ========================================================================
    // Mip 0, 1, 2
    //
    // One group covers:
    //
    // source : 64 x 64
    // mip1   : 32 x 32
    // mip2   : 16 x 16
    // ========================================================================
    uint2 tile_idx = group_id.xy;

    uint2 base_coord = tile_idx * 64u + thread_2d * 4u;

    float mip1_vals[4];

    uint2 mip0_size = GetMipSize(0);
    uint2 mip1_size = GetMipSize(1);
    uint2 mip2_size = GetMipSize(2);

    for (uint i = 0; i < 4; ++i){
        uint2 offset = uint2( (i & 1u) * 2u, (i >> 1u) * 2u);

        uint2 p = base_coord + offset;

        // ------------------------------------------------------------
        // Load source / visibility buffer
        // ------------------------------------------------------------

        float d00 = fetch_depth_from_visiblity_buffer(p + uint2(0, 0));
        float d10 = fetch_depth_from_visiblity_buffer(p + uint2(1, 0));
        float d01 = fetch_depth_from_visiblity_buffer(p + uint2(0, 1));
        float d11 = fetch_depth_from_visiblity_buffer(p + uint2(1, 1));

        // ------------------------------------------------------------
        // Mip0
        //
        // IMPORTANT:
        // Do bounds check BEFORE indexing the UAV.
        // ------------------------------------------------------------

        uint2 p00 = p + uint2(0, 0);
        uint2 p10 = p + uint2(1, 0);
        uint2 p01 = p + uint2(0, 1);
        uint2 p11 = p + uint2(1, 1);

        if (InBounds(p00, mip0_size)) occlusion_hiz[0][p00] = d00;
        if (InBounds(p10, mip0_size)) occlusion_hiz[0][p10] = d10;
        if (InBounds(p01, mip0_size)) occlusion_hiz[0][p01] = d01;
        if (InBounds(p11, mip0_size)) occlusion_hiz[0][p11] = d11;

        // ------------------------------------------------------------
        // Mip1
        // ------------------------------------------------------------

        float mip1 = ReduceMax4(d00, d10, d01, d11);

        uint2 mip1_coord = tile_idx * 32u + thread_2d * 2u + uint2(i & 1u, i >> 1u);

        if (InBounds(mip1_coord, mip1_size)) occlusion_hiz[1][mip1_coord] = mip1;

        mip1_vals[i] = mip1;
    }

    // ========================================================================
    // Mip2
    // ========================================================================

    float mip2_val = ReduceMax4(mip1_vals[0], mip1_vals[1], mip1_vals[2], mip1_vals[3]);

    uint2 mip2_coord = tile_idx * 16u + thread_2d;

    if (InBounds(mip2_coord, mip2_size)) occlusion_hiz[2][mip2_coord] = mip2_val;

    // LDS:
    // 16 x 16 = Mip2 produced by this workgroup
    gs_depth_samples[thread_2d.y][thread_2d.x] = mip2_val;

    GroupMemoryBarrierWithGroupSync();


    // ========================================================================
    // Mip3
    //
    // 16x16 -> 8x8
    //
    // FIX: 元コードは同じ gs_depth_samples から読んで同じ配列に書き戻しており、
    // 読み込みと書き込みの間にバリアが無かったため、あるスレッドの読み込みが
    // 別スレッドの書き込み後の値を拾ってしまうレースコンディションがあった。
    // → 読み込み・計算を先にすべて終わらせ、全スレッド一様なバリアを挟んでから
    //   書き込みを行うよう2段階に分離する。
    // ========================================================================

    uint2 mip3_size = GetMipSize(3);

    bool is_mip3_thread = thread_idx < 64;
    uint2 t2d_mip3 = uint2(thread_idx & 7u, thread_idx >> 3u);
    float mip3_val = 0.0f;

    if (is_mip3_thread){
        float v0 = gs_depth_samples[t2d_mip3.y * 2 + 0][t2d_mip3.x * 2 + 0];
        float v1 = gs_depth_samples[t2d_mip3.y * 2 + 0][t2d_mip3.x * 2 + 1];
        float v2 = gs_depth_samples[t2d_mip3.y * 2 + 1][t2d_mip3.x * 2 + 0];
        float v3 = gs_depth_samples[t2d_mip3.y * 2 + 1][t2d_mip3.x * 2 + 1];

        mip3_val = ReduceMax4(v0, v1, v2, v3);

        uint2 mip3_coord = tile_idx * 8u + t2d_mip3;

        if (InBounds(mip3_coord, mip3_size)) occlusion_hiz[3][mip3_coord] = mip3_val;
    }

    // 全スレッド (256) が一様に到達するバリア。
    // ここで、Mip2 の読み込みが全スレッド分完了してから初めて
    // 下の書き込み(上書き)が始まることを保証する。
    GroupMemoryBarrierWithGroupSync();

    if (is_mip3_thread){
        // Keep only 8x8 in LDS.
        gs_depth_samples[t2d_mip3.y][t2d_mip3.x] = mip3_val;
    }

    GroupMemoryBarrierWithGroupSync();


    // ========================================================================
    // Mip4
    //
    // 8x8 -> 4x4
    //
    // FIX: 元コードは occlusion_hiz_mip4 (クロスワークグループ集約用の専用リソース)
    // にしか書いておらず、コメントで警告されている通り occlusion_hiz[4] への
    // 書き込みが漏れていた。統一Mipチェーンを読む後続処理（オクルージョンテスト等）が
    // 未初期化データを読んでしまうため、両方に書き込む。
    // ========================================================================

    uint2 mip4_size = GetMipSize(4);

    if (thread_idx < 16) {
        uint2 t2d = uint2( thread_idx & 3u, thread_idx >> 2u);

        float v0 = gs_depth_samples[t2d.y * 2 + 0][t2d.x * 2 + 0];
        float v1 = gs_depth_samples[t2d.y * 2 + 0][t2d.x * 2 + 1];
        float v2 = gs_depth_samples[t2d.y * 2 + 1][t2d.x * 2 + 0];
        float v3 = gs_depth_samples[t2d.y * 2 + 1][t2d.x * 2 + 1];

        float mip4_val = ReduceMax4(v0, v1, v2, v3);
        uint2 mip4_coord = tile_idx * 4u + t2d;
        if (InBounds(mip4_coord, mip4_size)) {
            //occlusion_hiz[4][mip4_coord]   = mip4_val; // 統一Mipチェーン側 (Phase4が参照)
            occlusion_hiz_mip4[mip4_coord] = mip4_val; // クロスワークグループ集約用 (globallycoherent)
        }
    }

    GroupMemoryBarrierWithGroupSync();


    // ========================================================================
    // Global synchronization
    // ========================================================================


    if (thread_idx == 0) {
        InterlockedAdd( spd_completed_count[0], 1u, gs_spd_counter);
    }

    GroupMemoryBarrierWithGroupSync();

    // Only the last workgroup continues.
    uint total_work_groups = ((IMAGE_SIZE.x + 63u) / 64u) * ((IMAGE_SIZE.y + 63u) / 64u);
    if (gs_spd_counter != total_work_groups - 1u) return;

    // ========================================================================
    // Mip5 -> Mip11
    // ========================================================================

    uint2 current_size = GetMipSize(4);

    for (uint mip = 5; mip <= 11; ++mip) {
        uint2 dst_size = max( (current_size + 1u) >> 1u, uint2(1, 1));

        uint total_pixels = dst_size.x * dst_size.y;

        for ( uint i = thread_idx; i < total_pixels; i += THREAD_NUM) {
            uint2 dst_coord = uint2( i % dst_size.x, i / dst_size.x);

            uint2 src_coord = dst_coord * 2u;

            float v0, v1, v2, v3;
            if(mip == 5){ // Mip4 は専用 resource
                v0 = FetchMip4(src_coord + uint2(0, 0));
                v1 = FetchMip4(src_coord + uint2(1, 0));
                v2 = FetchMip4(src_coord + uint2(0, 1));
                v3 = FetchMip4(src_coord + uint2(1, 1)); 
            }else{
                v0 = FetchMipDepth( src_coord + uint2(0, 0), mip - 1);
                v1 = FetchMipDepth( src_coord + uint2(1, 0), mip - 1);
                v2 = FetchMipDepth( src_coord + uint2(0, 1), mip - 1);
                v3 = FetchMipDepth( src_coord + uint2(1, 1), mip - 1);
            }

            float result = ReduceMax4(v0, v1, v2, v3);
            occlusion_hiz[mip][dst_coord] = result;
        }

        // All threads in THIS final workgroup must finish
        // before the next mip reads this mip.
        AllMemoryBarrierWithGroupSync();

        current_size = dst_size;
    }

    // Counter reset
    if (thread_idx == 0) spd_completed_count[0] = 0;
}
*/

// グループ共有メモリ: SPD完了カウンタ用のみ (Mip3/Mip4の交換はWave Intrinsicsで
// レーン間直接交換するため、gs_depth_samples[16][16] は不要になった)
groupshared uint32_t gs_spd_counter;

//------------------------------------------------------------------------------
// ヘルパー関数: 2x2 の Depth サンプルから最大値（最も奥＝遮蔽の弱いDepth）を抽出
//------------------------------------------------------------------------------
float ReduceMax4(float d0, float d1, float d2, float d3) {
    return max(max(d0, d1), max(d2, d3));
}

//------------------------------------------------------------------------------
// Morton(Z字)順のスレッド配置
//
// Quad Intrinsics (QuadReadAcrossX/Y/Diagonal) は「wave内で4連続するレーン」を
// ハードウェア的に2x2の並びとして扱う(lane%4の下位2bitがX,Yに対応)。
// 通常の行優先(row-major)配置だと連続する4レーンは横一列(4x1)になってしまい、
// Quad操作の「隣」が空間的に隣接しないため、Mortonビットインターリーブで
// スレッド番号を2次元座標に変換する。
//
// 性質: thread_idx の下位2bit(bit0,bit1) = (x,y)それぞれの下位1bit
//       -> QuadReadAcrossX/Y/Diagonal がそのまま正しい空間的近傍になる。
//       この性質は再帰的に成り立つ(下位4bit=2x2quadの中の位置、
//       次の2bit=どのquadか、等)ので、後段のMip4計算でも利用する。
//------------------------------------------------------------------------------
uint MortonCompact1By1(uint n){
    n &= 0x55555555u;
    n = (n ^ (n >> 1))  & 0x33333333u;
    n = (n ^ (n >> 2))  & 0x0f0f0f0fu;
    n = (n ^ (n >> 4))  & 0x00ff00ffu;
    n = (n ^ (n >> 8))  & 0x0000ffffu;
    return n;
}

uint2 MortonDecode16x16(uint morton){
    return uint2(MortonCompact1By1(morton), MortonCompact1By1(morton >> 1));
}

#define IMAGE_SIZE uint32_t2(frame_render_data.render_target_width, frame_render_data.render_target_height)

// 64bit Visibility Buffer から Depth (float) を復元
float fetch_depth_from_visiblity_buffer(uint2 pos) {
    // 画面外アクセスの安全対策
    if (any(pos >= IMAGE_SIZE)) {
        return 1.0f; // Far Depth (Reversed-Z の場合は 0.0f に調整)
    }
    uint64_t temp = visiblity_buffer[pos];
    uint32_t ud = uint32_t(temp >> 33);
    return asfloat(ud);
}

// ============================================================================
// Mip size:  ceil(size / 2^mip)
// ============================================================================
uint2 GetMipSize(uint mip){
    uint2 size = IMAGE_SIZE;
    for (uint i = 0; i < mip; ++i){
        size = max((size + 1u) >> 1u, uint2(1, 1));
    }
    return size;
}

bool InBounds(uint2 p, uint2 size){
    return all(p < size);
}

float FetchMipDepth(uint2 pos, uint mip){
    uint2 size = GetMipSize(mip);
    if (!InBounds(pos, size)) return 1.0f;
    return occlusion_hiz[mip][pos];
}

float FetchMip4(uint2 pos){
    uint2 size = GetMipSize(4);
    if (any(pos >= size)) return 1.0f;
    return occlusion_hiz_mip4[pos];
}

// ============================================================================
// Main
// ============================================================================

#define THREAD_NUM 256
[shader("compute")]
[numthreads(THREAD_NUM, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id  : SV_GroupThreadID,
    in uint3 group_id         : SV_GroupID){

    uint thread_idx = local_thread_id.x;

    // FIX: 行優先ではなくMorton順に変更 (Quad Intrinsicsと整合させるため)
    uint2 thread_2d = MortonDecode16x16(thread_idx);

    // ========================================================================
    // Mip 0, 1, 2 (スレッド内で完結。ここは元々Wave Intrinsicsの余地なし)
    // ========================================================================
    uint2 tile_idx = group_id.xy;

    uint2 base_coord = tile_idx * 64u + thread_2d * 4u;

    float mip1_vals[4];

    uint2 mip0_size = GetMipSize(0);
    uint2 mip1_size = GetMipSize(1);
    uint2 mip2_size = GetMipSize(2);

    for (uint i = 0; i < 4; ++i){
        uint2 offset = uint2( (i & 1u) * 2u, (i >> 1u) * 2u);
        uint2 p = base_coord + offset;

        float d00 = fetch_depth_from_visiblity_buffer(p + uint2(0, 0));
        float d10 = fetch_depth_from_visiblity_buffer(p + uint2(1, 0));
        float d01 = fetch_depth_from_visiblity_buffer(p + uint2(0, 1));
        float d11 = fetch_depth_from_visiblity_buffer(p + uint2(1, 1));

        uint2 p00 = p + uint2(0, 0);
        uint2 p10 = p + uint2(1, 0);
        uint2 p01 = p + uint2(0, 1);
        uint2 p11 = p + uint2(1, 1);

        if (InBounds(p00, mip0_size)) occlusion_hiz[0][p00] = d00;
        if (InBounds(p10, mip0_size)) occlusion_hiz[0][p10] = d10;
        if (InBounds(p01, mip0_size)) occlusion_hiz[0][p01] = d01;
        if (InBounds(p11, mip0_size)) occlusion_hiz[0][p11] = d11;

        float mip1 = ReduceMax4(d00, d10, d01, d11);
        uint2 mip1_coord = tile_idx * 32u + thread_2d * 2u + uint2(i & 1u, i >> 1u);
        if (InBounds(mip1_coord, mip1_size)) occlusion_hiz[1][mip1_coord] = mip1;

        mip1_vals[i] = mip1;
    }

    float mip2_val = ReduceMax4(mip1_vals[0], mip1_vals[1], mip1_vals[2], mip1_vals[3]);

    uint2 mip2_coord = tile_idx * 16u + thread_2d;
    if (InBounds(mip2_coord, mip2_size)) occlusion_hiz[2][mip2_coord] = mip2_val;

    // ========================================================================
    // Mip3 (16x16 -> 8x8) : Quad Intrinsicsでレーン間直接交換
    //
    // FIX: 旧実装はLDSに書いてバリアを挟んで読み直していた
    // (かつては読み書きが同じ配列で競合するバグもあった箇所)。
    // Quad Intrinsicsは1命令でquad内の値を直接交換できるため、
    // LDSもバリアも不要になる。
    // ========================================================================
    uint2 mip3_size = GetMipSize(3);

    float mip3_val = ReduceMax4(
        mip2_val,
        QuadReadAcrossX(mip2_val),
        QuadReadAcrossY(mip2_val),
        QuadReadAcrossDiagonal(mip2_val)
    );

    // quad内の4スレッドは同じmip3_valを計算するので、代表1スレッドだけが書く
    bool is_quad_leader = all((thread_2d & 1u) == 0u);
    if (is_quad_leader){
        uint2 mip3_coord = tile_idx * 8u + (thread_2d >> 1u);
        if (InBounds(mip3_coord, mip3_size)) occlusion_hiz[3][mip3_coord] = mip3_val;
    }

    // ========================================================================
    // Mip4 (8x8 -> 4x4) : WaveReadLaneAt によるバタフライ交換
    //
    // Quad Intrinsicsは直接隣り合う2x2(quad)しか読めないため、その先
    // (2つ隣のquad)を読むには、wave内の相対レーン番号をXORした位置を
    // 明示的に読む(バタフライシャッフル)。
    //
    // 重要: thread_idx同士をXORするのではなく、必ず WaveGetLaneIndex()
    // (wave内の相対番号)をXORすること。thread_idxをそのまま使うと、
    // 2つ目以降のwaveでは対応するレーンがずれて誤った値を読んでしまう。
    // (wave幅が16以上の2べき、かつthread_idxがレーンへ連番で
    // 割り当てられることが前提。主要なデスクトップ/コンソールGPU
    // (wave32/wave64)ではこの前提は満たされる)
    // ========================================================================
    uint2 mip4_size = GetMipSize(4);

    uint lane = WaveGetLaneIndex();
    float mip4_val = ReduceMax4(
        mip3_val,
        WaveReadLaneAt(mip3_val, lane ^ 4u),
        WaveReadLaneAt(mip3_val, lane ^ 8u),
        WaveReadLaneAt(mip3_val, lane ^ 12u)
    );

    bool is_block_leader = all((thread_2d & 3u) == 0u);
    if (is_block_leader){
        uint2 mip4_coord = tile_idx * 4u + (thread_2d >> 2u);
        if (InBounds(mip4_coord, mip4_size)) {
            occlusion_hiz[4][mip4_coord]   = mip4_val; // 統一Mipチェーン側 (Phase4が参照)
            occlusion_hiz_mip4[mip4_coord] = mip4_val; // クロスワークグループ集約用 (globallycoherent)
        }
    }

    // NOTE: Mip3/Mip4を計算するのに GroupMemoryBarrierWithGroupSync は
    // 一切不要になった(Wave内の全レーンはロックステップで実行されるため、
    // 分岐を挟まない限りQuad/Wave操作の前後で明示的な同期は不要)。


    // ========================================================================
    // Global synchronization (Mip5以降の集約はワークグループをまたぐため、
    // ここは引き続きLDS+atomicカウンタが必要。Wave Intrinsicsでは代替できない)
    // ========================================================================
    if (thread_idx == 0) {
        InterlockedAdd( spd_completed_count[0], 1u, gs_spd_counter);
    }

    GroupMemoryBarrierWithGroupSync();

    uint total_work_groups = ((IMAGE_SIZE.x + 63u) / 64u) * ((IMAGE_SIZE.y + 63u) / 64u);
    if (gs_spd_counter != total_work_groups - 1u) return;

    // ========================================================================
    // Mip5 -> Mip11
    //
    // NOTE: ここは各スレッドが独立したUAV/テクスチャ読み出しを行っており、
    // 隣接スレッド間で同じデータを再利用する場面が無いため、
    // Wave Intrinsicsによる削減の余地は無い(ハードウェアのテクスチャ
    // キャッシュに任せるのが適切)。
    // ========================================================================
    uint2 current_size = GetMipSize(4);

    for (uint mip = 5; mip <= 11; ++mip) {
        uint2 dst_size = max( (current_size + 1u) >> 1u, uint2(1, 1));
        uint total_pixels = dst_size.x * dst_size.y;

        for ( uint i = thread_idx; i < total_pixels; i += THREAD_NUM) {
            uint2 dst_coord = uint2( i % dst_size.x, i / dst_size.x);
            uint2 src_coord = dst_coord * 2u;

            float v0, v1, v2, v3;
            if(mip == 5){
                v0 = FetchMip4(src_coord + uint2(0, 0));
                v1 = FetchMip4(src_coord + uint2(1, 0));
                v2 = FetchMip4(src_coord + uint2(0, 1));
                v3 = FetchMip4(src_coord + uint2(1, 1)); 
            }else{
                v0 = FetchMipDepth( src_coord + uint2(0, 0), mip - 1);
                v1 = FetchMipDepth( src_coord + uint2(1, 0), mip - 1);
                v2 = FetchMipDepth( src_coord + uint2(0, 1), mip - 1);
                v3 = FetchMipDepth( src_coord + uint2(1, 1), mip - 1);
            }

            float result = ReduceMax4(v0, v1, v2, v3);
            occlusion_hiz[mip][dst_coord] = result;
        }

        AllMemoryBarrierWithGroupSync();
        current_size = dst_size;
    }

    if (thread_idx == 0) spd_completed_count[0] = 0;
}