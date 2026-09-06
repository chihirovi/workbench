#pragma once

struct ProjectionBoundingSphereInfo{
    float2 pixel_min;
    float2 pixel_max;
    float2 ndc_near_far; // x: 最近接NDC-Z, y: 最遠NDC-Z
};

bool CoversPixelCenter(ProjectionBoundingSphereInfo info){
    float2 first_center = ceil(info.pixel_min - 0.5) + 0.5;
    return all(first_center <= info.pixel_max);
}

// ============================================================================
// HiZピラミッドの各ミップサイズ算出（前回作成したSPDシェーダの GetMipSize と同じロジック）
// ============================================================================
/*
uint2 GetHizMipSize(uint2 frame_size, uint mip){
    uint2 size = frame_size;
    for (uint i = 0; i < mip; ++i){
        size = max((size + 1u) >> 1u, uint2(1, 1));
    }
    return size;
}
*/
uint2 GetHizMipSize(uint2 frame_size, uint mip){
    uint scale = 1u << mip;
    return max((frame_size + scale - 1u) >> mip, uint2(1, 1));
}

ProjectionBoundingSphereInfo projection_bounding_sphere(
    float4 bounding_sphere, 
    float4x4 mvp_mat,
    float2 frame_size,
    float near_plane){

    float3 center_world = bounding_sphere.xyz;
    float radius = bounding_sphere.w;

    float4 center_clip = mul(mvp_mat, float4(center_world, 1.0));

    ProjectionBoundingSphereInfo info;

    // ------------------------------------------------------------
    // 1. カメラ空間でのZ深度チェック（近平面との干渉回避）
    //
    // FIX: 元コードはコメントだけで実装が無かった。
    // center_clip.w は（標準的な透視投影行列であれば）カメラ前方向への
    // 視点空間距離に相当する。球がニアプレーンを跨ぐ、またはカメラの
    // 後ろにある場合、center_clip.xy / center_clip.w による NDC 変換は
    // 符号反転・発散して正しく扱えないため、保守的に「画面全体を覆って
    // いる」ものとして扱い、以降の計算をスキップする。
    // ------------------------------------------------------------
    float2 ndc_min; 
    float2 ndc_max;
    if (center_clip.w - radius < near_plane){
        ndc_min = float2(-1.0, -1.0);
        ndc_max = float2( 1.0,  1.0);
        info.ndc_near_far = float2(0.0, 1.0);
        return info;
    }

    // ------------------------------------------------------------
    // 2. 画面上のAABB (NDC空間) を算出
    //
    // FIX: 元コードは `radius / center_clip.w` のみで近似しており、
    // 射影行列のスケール(P00, P11。FOV/アスペクト比に依存)を全く
    // 考慮していなかったため、投影サイズが実際と異なっていた。
    // view行列の回転部分は直交行列（各行の長さ=1）なので、
    // mvp_mat の各行(xyz成分)の長さがそのまま射影スケール |P00|,|P11|
    // に一致する。これによりカメラの回転に関わらず正しく補正できる。
    // ------------------------------------------------------------
    float proj_scale_x = length(mvp_mat[0].xyz);
    float proj_scale_y = length(mvp_mat[1].xyz);

    float2 ndc_center  = center_clip.xy / center_clip.w;
    float2 proj_radius = radius * float2(proj_scale_x, proj_scale_y) / center_clip.w;

    ndc_min = ndc_center - proj_radius;
    ndc_max = ndc_center + proj_radius;

    // ------------------------------------------------------------
    // 3. 深度方向(NDC Z)の範囲
    //
    // FIX: 元コードは ndc_near_far が常に (0.0, 1.0) 固定で、
    // 球の実際の深度分布を全く反映していなかった
    // （frustum_culling側もこのフィールドを未使用だった）。
    //
    // カメラ前方方向を mvp_mat の4行目(w成分を計算する行)の xyz から
    // 復元する。一般的な透視投影行列は proj[3] = [0,0,±1,0] であり、
    // mvp_mat[3].xyz は view行列の第3行＝カメラ前方向ベクトルに一致する。
    // 球の最近接点・最遠点をこの方向にradiusだけずらして実際に
    // mvpで再投影し、NDC Zを求める。
    //
    // 注意: 符号規則（右手/左手系、Reversed-Zの有無）によっては
    // 前方向の符号や比較の向きが変わる可能性があるため、実際の
    // プロジェクトの射影行列の規約と必ず突き合わせて確認すること。
    // ------------------------------------------------------------
    float3 view_dir = normalize(mvp_mat[3].xyz);

    float4 near_point_clip = mul(mvp_mat, float4(center_world - view_dir * radius, 1.0));
    float4 far_point_clip  = mul(mvp_mat, float4(center_world + view_dir * radius, 1.0));

    float near_z = near_point_clip.z / max(near_point_clip.w, 1e-5);
    float far_z  = far_point_clip.z  / max(far_point_clip.w,  1e-5);

    info.ndc_near_far = float2(min(near_z, far_z), max(near_z, far_z));

    float2 uv_a = ndc_min * 0.5 + 0.5;
    float2 uv_b = ndc_max * 0.5 + 0.5;

    float2 uv_min = saturate(min(uv_a, uv_b));
    float2 uv_max = saturate(max(uv_a, uv_b));

    info.pixel_min = uv_min * float2(frame_size);
    info.pixel_max = uv_max * float2(frame_size);

    return info;
}

// ============================================================================
// Gribb–Hartmann 法によるワールド空間視錐台カリング (6平面判定)
// ============================================================================
bool frustum_culling(float4 bounding_sphere, float4x4 mvp_mat) {
    float3 center = bounding_sphere.xyz;
    float radius  = bounding_sphere.w;

    // MVP行列の「行(Row)」からワールド空間の視錐台6平面を抽出
    // HLSLでは mvp_mat[i] が Row i に相当
    float4 planes[6];
    
    // Left / Right
    planes[0] = mvp_mat[3] + mvp_mat[0];
    planes[1] = mvp_mat[3] - mvp_mat[0];
    // Bottom / Top
    planes[2] = mvp_mat[3] + mvp_mat[1];
    planes[3] = mvp_mat[3] - mvp_mat[1];
    // Near / Far (D3D/Vulkan [0, 1] 深度レンジ前提)
    planes[4] = mvp_mat[2];
    planes[5] = mvp_mat[3] - mvp_mat[2];

    for (int i = 0; i < 6; ++i) {
        // 平面方程式 (Ax + By + Cz + D = 0) の正規化
        float length_inv = rsqrt(dot(planes[i].xyz, planes[i].xyz));
        planes[i] *= length_inv;

        // 球の中心から平面までの符号付き距離を計算
        // 距離が -radius 未満であれば、球は完全に平面の外側（視錐台外）にある
        if (dot(planes[i].xyz, center) + planes[i].w < -radius) {
            return false; // カリング
        }
    }

    return true; // 視錐台内部（または交差）
}

// ------------------------------------------------------------
// AABB全体が「1 HiZ texel」に収まるmipを選択
//
// mip 0 : 1x1 pixel
// mip 1 : 2x2 pixel
// mip 2 : 4x4 pixel
// mip 3 : 8x8 pixel
// ...
//
// 例えば AABBが 100 pixel 幅なら
// mip = ceil(log2(100)) = 7
// -> 128x128 pixel相当の1 texel
// ------------------------------------------------------------

// ============================================================================
// pixel_size(>=1) を覆うのに必要な最小ミップレベル = ceil(log2(pixel_size))
//
// firstbithigh(x) は floor(log2(x)) を返すため、そのまま使うと mip が
// 1段階足りず(テクセルがAABBより小さくなり)、隙間越しに見えるポリゴンの
// 判定が不安定になってちらつく原因になる。
// firstbithigh(dim-1)+1 とすることで ceil(log2(dim)) と等価になる
// (dim=1のときだけ特別扱いしてmip=0)。
// ============================================================================
uint PixelSizeToMip(uint dim){
    return (dim <= 1u) ? 0u : (firstbithigh(dim - 1u) + 1u);
}

// ============================================================================
// HiZ occlusion culling (Two-Phase Occlusion Culling 用)
//
// 前提: hiz[] は各テクセルが「その範囲内で最も“奥”(遮蔽の弱い)Depth」を
// 保持するように構築されていること (通常Z: near=0,far=1 なら max
// reduction。Reversed-Zならmin reduction)。
//
// FIX: 以前の2案はどちらも「AABB全体を正しくカバーできていない」ケースが
// あり、隙間越しに見える奥のポリゴンでカメラを動かすとちらつく原因に
// なっていた。
//  - 1テクセルだけ読む案 (hiz_occlusion_culling2 相当): AABBがテクセル
//    境界をまたぐと、隙間側/遮蔽側のどちらを拾うかがサブピクセル単位の
//    位置ズレで毎フレーム変わってしまう。
//  - firstbithigh(dim) を使う案: floor(log2(dim))相当になってしまい
//    ceilが必要な場面でミップが1段階細かすぎ(テクセルがAABBより小さく)
//    なり、4隅サンプリングでも中間のテクセルを取りこぼす。
// → mip選択を ceil(log2(dim)) 相当に修正した上で、必ず4隅をサンプルする
//   1つの実装に統合する。
// ============================================================================
bool hiz_occlusion_culling3(
    ProjectionBoundingSphereInfo info,
    uint2 frame_size,
    RWTexture2D<float32_t> hiz[12]){

    float2 pixel_size = max(info.pixel_max - info.pixel_min, float2(1.0, 1.0));

    // FIX: ceil(log2(dim)) と等価な整数演算に修正
    uint dim = uint(ceil(max(pixel_size.x, pixel_size.y)));
    uint mip = clamp(PixelSizeToMip(dim), 1u, 11u);

    uint2 mip_size = GetHizMipSize(frame_size, mip);

    uint2 texel_min = uint2(floor(info.pixel_min)) >> mip;
    uint2 texel_max = uint2(ceil(info.pixel_max))  >> mip;

    texel_min = min(texel_min, mip_size - 1u);
    texel_max = min(texel_max, mip_size - 1u);

    // FIX: 常に4隅をサンプルする(1テクセルだけに省略しない)。
    // mip選択がceilになったことで、このAABBは最大でも2x2テクセルに
    // 収まることが保証されるため、4隅で全域を過不足なくカバーできる。

    float occluder_depth;
    if (all(texel_min == texel_max)){
    //if (false){
        // AABBが単一テクセルに収まっている(多くの場合こちら) -> 1回のフェッチ
        occluder_depth = hiz[mip][texel_min];
    }else{
        float d0 = hiz[mip][uint2(texel_min.x, texel_min.y)];
        float d1 = hiz[mip][uint2(texel_max.x, texel_min.y)];
        float d2 = hiz[mip][uint2(texel_min.x, texel_max.y)];
        float d3 = hiz[mip][uint2(texel_max.x, texel_max.y)];
        occluder_depth = max(max(d0, d1), max(d2, d3));
    }

    float object_near_depth = info.ndc_near_far.x;

    bool occluded = object_near_depth > occluder_depth;

    return !occluded; // true = 描画すべき(遮蔽されていない)
}

/*
AABBの幅がたった2ピクセル（x = [127, 129]）だとします。
「この程度小さければmip1〜2で十分収まるはず」と思いますよね。実際に境界をまたぐかどうかを追ってみます（T = 2^mip がテクセルサイズ）。

mip	テクセルサイズ T	127が入るテクセル   129が入るテクセル	またぐ？
1	2	               63	             64	                またぐ
2	4	               31	             32	                またぐ
3	8	               15	             16	                またぐ
4	16	               7	             8	                またぐ
5	32	               3	             4	                またぐ
6	64	               1	             2	                またぐ
7	128	               0	             1	                またぐ
8	256	               0	             0	                収まる
*/