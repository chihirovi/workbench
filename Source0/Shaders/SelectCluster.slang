#include "Include/GlobalDEscriptor.h.slang"
#include "Include/Culling.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"

/*
// こっちでは距離の二乗で割ってる
auto ErrorProjection = [&](const DirectX::XMFLOAT3& center, float error)
{
    float yScale = 1.0f / tanf(InFovYRad * 0.5f);
    float d2 = (center.x * center.x) + (center.y * center.y) + (center.z * center.z);
    return InScreenHeight * 0.5f * yScale * error / (d2 - error * error);
};
*/
/*
float lod_error_is_imperceptible(
    float4 lod_sphere, 
    float simplification_error, 
    float4x4 world_from_local, 
    float world_scale) {

    let sphere_world_space = mul(world_from_local, float4(lod_sphere.xyz, 1.0)).xyz;
    let radius_world_space = world_scale * lod_sphere[3];
    let error_world_space = world_scale * simplification_error;

    var projected_error = error_world_space;
    // Perspective
    let distance_to_closest_point_on_sphere = distance(sphere_world_space, frame_render_data.camera_position.xyz) - radius_world_space;
    let distance_to_closest_point_on_sphere_clamped_to_znear = max(distance_to_closest_point_on_sphere, frame_render_data.projection_matrix[3][2]);
    
    // lod の減衰が遅いから，二乗で割る
    projected_error /= distance_to_closest_point_on_sphere_clamped_to_znear;
    //projected_error /= distance_to_closest_point_on_sphere_clamped_to_znear * distance_to_closest_point_on_sphere_clamped_to_znear;

    projected_error *= -frame_render_data.projection_matrix[1][1] * 0.5;
    projected_error *= frame_render_data.render_target_height;

    return projected_error;
}

// AABBのまま期待値（平均投影ピクセル数）を計算する関数
float CalculateAABBProjectedPixelsExact(
    float3 aabb_min, 
    float3 aabb_max, 
    float4x4 model_matrix,
    GPU_FrameRenderData _frame_render_data){
    // -------------------------------------------------------------
    // 1. AABBの3辺の長さ (dx, dy, dz) をワールド空間スケールで算出
    // -------------------------------------------------------------
    float3 local_size = aabb_max - aabb_min; // (dx, dy, dz)

    // ワールド行列の各軸のスケールを取得
    float3 world_scale = float3(
        length(model_matrix[0].xyz),
        length(model_matrix[1].xyz),
        length(model_matrix[2].xyz)
    );
    
    // ワールド空間での各辺長
    float3 S = local_size * world_scale; // S.x = dx, S.y = dy, S.z = dz

    // -------------------------------------------------------------
    // 2. AABB中心位置とカメラからのZ深度 d の計算
    // -------------------------------------------------------------
    float3 local_center = (aabb_min + aabb_max) * 0.5;
    float3 world_center = mul(model_matrix, float4(local_center, 1.0)).xyz;

    // 視線方向ベクトルに射影して深度 d を算出
    float3 view_vec = world_center - _frame_render_data.camera_position.xyz;
    float d = dot(view_vec, normalize(_frame_render_data.camera_lookat_d.xyz));

    // カメラの背面にある場合は 0
    if (d <= 0.001) return 0.0;

    // -------------------------------------------------------------
    // 3. プロジェクション行列からピクセル単位の焦点距離 (fx, fy) を抽出
    // -------------------------------------------------------------
    // projection_matrix[0][0] = 2 * fx / width
    // projection_matrix[1][1] = 2 * fy / height
    float fx = _frame_render_data.projection_matrix[0][0] * 0.5 * (float)_frame_render_data.render_target_width;
    float fy = -1.0f * _frame_render_data.projection_matrix[1][1] * 0.5 * (float)_frame_render_data.render_target_height;

    // -------------------------------------------------------------
    // 4. AABBの期待投影面積 (ワールド空間) と スクリーンピクセル数の算出
    // -------------------------------------------------------------
    // AABB表面積の1/4 = (dx*dy + dy*dz + dz*dx) / 2
    float world_projected_area_avg = (S.x * S.y + S.y * S.z + S.z * S.x) * 0.5;

    // 透視変換による縮小率 (fx * fy) / d^2 を乗算してピクセル数を導出
    float pixel_area = world_projected_area_avg * (fx * fy) / (d * d);

    return pixel_area;
}

bool is_soft_rasterize(
    Cluster cluster,
    float4x4 model_matrix,
    GPU_FrameRenderData _frame_render_data){
    
    float screen_pixel_area = CalculateAABBProjectedPixelsExact(
        float3(cluster.self_aabb[0], cluster.self_aabb[1], cluster.self_aabb[2]),
        float3(cluster.self_aabb[3], cluster.self_aabb[4], cluster.self_aabb[5]),
        model_matrix,
        _frame_render_data
    );

    if(screen_pixel_area < 0.0f) return false;

    // screen_pixel_area/float(cluster.triangle_count) < 1.0f の式変形
    // 1 triangle を占めるピクセル数
    //return screen_pixel_area < float(cluster.triangle_count);
    return screen_pixel_area < float(cluster.triangle_count) * 3.0f;
}

// グループ全体で合算するための共有メモリ
groupshared uint g_groupActiveCountHWRaster;
groupshared uint g_groupBaseIndexHWRaster;
groupshared uint g_groupActiveCountSWRaster;
groupshared uint g_groupBaseIndexSWRaster;

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
    
    // freeze culling
    if(frame_render_data.editor_data[4] == 1){
        return;
    }
    
    // 
    bool is_active = true;
    uint32_t thread_idx = global_thread_id.x;
    if(cluster_num_prefixsum[frame_render_data.virtual_geometry_opaque_instance_ref_range.instance_ref_count] <= global_thread_id.x){
        is_active = false;
        thread_idx = cluster_num_prefixsum[
            frame_render_data.virtual_geometry_opaque_instance_ref_range.instance_ref_count
        ] - 1;
    }
    
    // 二分探索
    uint32_t low = 0;
    uint32_t high = frame_render_data.virtual_geometry_opaque_instance_ref_range.instance_ref_count;

    while (low < high){
        uint mid = (low + high) / 2;
        if (cluster_num_prefixsum[mid + 1] <= thread_idx){
            low = mid + 1;
        }else{
            high = mid;
        }
    }
    
    uint32_t virtual_ref_idx = low;
    uint32_t cluster_num_sum = cluster_num_prefixsum[virtual_ref_idx];
    
    //
    uint32_t cluster_id = thread_idx - cluster_num_sum;
    InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_idx];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
    Cluster cluster = instance.cluster_addr[cluster_id];
    
    float threshold = 0.1;
    if(frame_render_data.editor_data[2] == 0){ // not auto
        if (cluster.lod_level != frame_render_data.editor_data[1]){
            is_active = false;
        }

    }else{ // auto lod selection
        
        float scale = max(max(inst_ref.model_matrix[0][0], inst_ref.model_matrix[1][1]), inst_ref.model_matrix[2][2]);

        float4 child_lod_sphere = float4( cluster.self_lod_bound_sphere[0], cluster.self_lod_bound_sphere[1], cluster.self_lod_bound_sphere[2], cluster.self_lod_bound_sphere[3]);
        float child_error = lod_error_is_imperceptible(child_lod_sphere, cluster.error_self, inst_ref.model_matrix, scale);
        
        float4 parent_lod_sphere = float4(cluster.parent_lod_bound_sphere[0], cluster.parent_lod_bound_sphere[1], cluster.parent_lod_bound_sphere[2], cluster.parent_lod_bound_sphere[3]);
        float parent_error = lod_error_is_imperceptible(parent_lod_sphere, cluster.error_parent, inst_ref.model_matrix, scale);
        
        if(child_error <= threshold && threshold < parent_error){}
        else is_active = false;
    }
    
    // ==========================
    // culling
    // ==========================
    float3 aabb_min = float3(cluster.self_aabb[0], cluster.self_aabb[1], cluster.self_aabb[2]);
    float3 aabb_max = float3(cluster.self_aabb[3], cluster.self_aabb[4], cluster.self_aabb[5]);
    float3 bs_center = (aabb_min + aabb_max)/2.0;
    float bs_r = length(aabb_max - bs_center);
    
    // frustum
    if(is_active){
        bool passed_frustum = frustum_culling(
            float4(bs_center, bs_r), 
            mul(frame_render_data.projection_view_matrix, inst_ref.model_matrix)
        );
        is_active &= passed_frustum;
    }

    // projection bs
    ProjectionBoundingSphereInfo bs_info = projection_bounding_sphere(
        float4(bs_center, bs_r),
        mul(frame_render_data.projection_view_matrix, inst_ref.model_matrix),
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        0.00f
    );
    
    // micro cluster culling
    bool is_micro_cluster = CoversPixelCenter(bs_info);
    is_active &= is_micro_cluster;

    // hiz

    bool passed_hiz = true;
    if(is_active){
        passed_hiz = hiz_occlusion_culling3(
            bs_info,
            uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
            occlusion_hiz
        );
    }
    let is_false_negative = !passed_hiz;

    // recode
    // SWRaster, HWRaster の振り分け
    bool is_soft_raster = is_soft_rasterize(cluster, inst_ref.model_matrix, frame_render_data);
    if(frame_render_data.editor_data[3] == 1){ 
        is_soft_raster = true;
    }else if(frame_render_data.editor_data[3] == 2){
        is_soft_raster = false;
    }
    let is_hard_raster = !is_soft_raster;
    
    // -------------------------------------------------------------
    // 1. Wave内の集計
    // -------------------------------------------------------------
    uint waveActiveCountHWRaster = WaveActiveCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveLocalIndexHWRaster  = WavePrefixCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveActiveCountSWRaster = WaveActiveCountBits(is_active & passed_hiz & is_soft_raster);
    uint waveLocalIndexSWRaster  = WavePrefixCountBits(is_active & passed_hiz & is_soft_raster);

    uint waveActiveCountHWRasterFN = WaveActiveCountBits(is_active & is_false_negative & is_hard_raster);
    uint waveLocalIndexHWRasterFN  = WavePrefixCountBits(is_active & is_false_negative & is_hard_raster);
    uint waveActiveCountSWRasterFN = WaveActiveCountBits(is_active & is_false_negative & is_soft_raster);
    uint waveLocalIndexSWRasterFN  = WavePrefixCountBits(is_active & is_false_negative & is_soft_raster);

    // 各Waveの先頭スレッド（Leader）が、グループ共有メモリにWaveごとのカウントを足し込む
    if (local_thread_id.x == 0)
    {
        g_groupActiveCountHWRaster = 0;
        g_groupActiveCountSWRaster = 0;
        g_groupActiveCountHWRasterFN = 0;
        g_groupActiveCountSWRasterFN = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint waveBaseInGroupHWRaster = 0;
    uint waveBaseInGroupSWRaster = 0;
    uint waveBaseInGroupHWRasterFN = 0;
    uint waveBaseInGroupSWRasterFN = 0;

    if (WaveIsFirstLane())
    {
        // 共有メモリに対するAtomic加算（高速・Wave間で集計）
        InterlockedAdd(g_groupActiveCountHWRaster, waveActiveCountHWRaster, waveBaseInGroupHWRaster);
        InterlockedAdd(g_groupActiveCountSWRaster, waveActiveCountSWRaster, waveBaseInGroupSWRaster);
        InterlockedAdd(g_groupActiveCountHWRasterFN, waveActiveCountHWRasterFN, waveBaseInGroupHWRasterFN);
        InterlockedAdd(g_groupActiveCountSWRasterFN, waveActiveCountSWRasterFN, waveBaseInGroupSWRasterFN);
    }

    // 全Waveによる共有メモリ(g_groupActiveCount)への加算完了を待つ
    GroupMemoryBarrierWithGroupSync();

    // Wave内のLeaderが得た「グループ内でのオフセット」をWave内の全スレッドへ共有
    waveBaseInGroupHWRaster = WaveReadLaneFirst(waveBaseInGroupHWRaster);
    waveBaseInGroupSWRaster = WaveReadLaneFirst(waveBaseInGroupSWRaster);
    waveBaseInGroupHWRasterFN = WaveReadLaneFirst(waveBaseInGroupHWRasterFN);
    waveBaseInGroupSWRasterFN = WaveReadLaneFirst(waveBaseInGroupSWRasterFN);

    // -------------------------------------------------------------
    // 2. グローバルAtomic Add（Thread 0 のみ実行）
    // -------------------------------------------------------------
    if (local_thread_id.x == 0)
    {
        // グループ全体の合計数をグローバルキューへ1回だけAtomic Add
        InterlockedAdd(cluster_render_queue[0].head_num, g_groupActiveCountHWRaster, g_groupBaseIndexHWRaster);
        InterlockedAdd(cluster_render_queue[0].tail_num, g_groupActiveCountSWRaster, g_groupBaseIndexSWRaster);
        InterlockedAdd(cluster_render_queue[0].HW_fn_count, g_groupActiveCountHWRasterFN, g_groupBaseIndexHWRasterFN);
        InterlockedAdd(cluster_render_queue[0].SW_fn_count, g_groupActiveCountSWRasterFN, g_groupBaseIndexSWRasterFN);
        
        // 2st select comp arg
        uint32_t tmp;
        InterlockedAdd(cluster_render_queue[0].fn_count, g_groupActiveCountHWRasterFN + g_groupActiveCountSWRasterFN, tmp);
        tmp = (tmp + CTA_NUM - 1) / CTA_NUM;
        InterlockedMax(cluster_render_queue[0].sx, tmp);
    }
    GroupMemoryBarrierWithGroupSync(); // グローバルIndexの取得完了待ち

    // -------------------------------------------------------------
    // 3. 書き込み位置の計算と書き込み
    // -------------------------------------------------------------
    // グローバル基準値 + グループ内Waveオフセット + Wave内スレッドオフセット
    if(is_active & passed_hiz & is_hard_raster){
        uint writeIndex = g_groupBaseIndexHWRaster + waveBaseInGroupHWRaster + waveLocalIndexHWRaster;
        if(writeIndex < MAX_RENDER_CLUSTER){
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }

    if(is_active & passed_hiz & is_soft_raster){
        uint writeIndex = g_groupBaseIndexSWRaster + waveBaseInGroupSWRaster + waveLocalIndexSWRaster;
        if(writeIndex < MAX_RENDER_CLUSTER){
            writeIndex = MAX_RENDER_CLUSTER - 1 - writeIndex;
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }
    
    // false negative
    if(is_active & is_false_negative & is_hard_raster){
        uint writeIndex = g_groupBaseIndexHWRasterFN + waveBaseInGroupHWRasterFN + waveLocalIndexHWRasterFN;
        if(writeIndex < MAX_RENDER_CLUSTER){
            cluster_render_queue[0].data_false_negative[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }

    if(is_active & is_false_negative & is_soft_raster){
        uint writeIndex = g_groupBaseIndexSWRasterFN + waveBaseInGroupSWRasterFN + waveLocalIndexSWRasterFN;
        if(writeIndex < MAX_RENDER_CLUSTER){
            writeIndex = MAX_RENDER_CLUSTER - 1 - writeIndex;
            cluster_render_queue[0].data_false_negative[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }
}
*/
    
// FIX: 同一ファイルの二重include (相対パス違い)を削除
// #include "../Include/GlobalDEscriptor.h.slang"

/*
// こっちでは距離の二乗で割ってる
auto ErrorProjection = [&](const DirectX::XMFLOAT3& center, float error)
{
    float yScale = 1.0f / tanf(InFovYRad * 0.5f);
    float d2 = (center.x * center.x) + (center.y * center.y) + (center.z * center.z);
    return InScreenHeight * 0.5f * yScale * error / (d2 - error * error);
};
*/
float lod_error_is_imperceptible(
    float4 lod_sphere, 
    float simplification_error, 
    float4x4 world_from_local, 
    float world_scale) {

    let sphere_world_space = mul(world_from_local, float4(lod_sphere.xyz, 1.0)).xyz;
    let radius_world_space = world_scale * lod_sphere[3];
    let error_world_space = world_scale * simplification_error;

    var projected_error = error_world_space;
    // Perspective
    let distance_to_closest_point_on_sphere = distance(sphere_world_space, frame_render_data.camera_position.xyz) - radius_world_space;
    let distance_to_closest_point_on_sphere_clamped_to_znear = max(distance_to_closest_point_on_sphere, frame_render_data.projection_matrix[3][2]);

    // NOTE: 以下のコメントと実際のコードが矛盾しています。
    // コメントは「二乗で割る」としていますが、有効な行は線形距離で割っており、
    // 二乗で割る行はコメントアウトされたままです。意図を確認してコメントを
    // 修正することをおすすめします(挙動はそのままにしています)。
    // lod の減衰が遅いから，二乗で割る
    projected_error /= distance_to_closest_point_on_sphere_clamped_to_znear;
    //projected_error /= distance_to_closest_point_on_sphere_clamped_to_znear * distance_to_closest_point_on_sphere_clamped_to_znear;

    projected_error *= -frame_render_data.projection_matrix[1][1] * 0.5;
    projected_error *= frame_render_data.render_target_height;

    return projected_error;
}

// AABBのまま期待値（平均投影ピクセル数）を計算する関数
float CalculateAABBProjectedPixelsExact(
    float3 aabb_min, 
    float3 aabb_max, 
    float4x4 model_matrix,
    GPU_FrameRenderData _frame_render_data){
    float3 local_size = aabb_max - aabb_min;

    float3 world_scale = float3(
        length(model_matrix[0].xyz),
        length(model_matrix[1].xyz),
        length(model_matrix[2].xyz)
    );
    
    float3 S = local_size * world_scale;

    float3 local_center = (aabb_min + aabb_max) * 0.5;
    float3 world_center = mul(model_matrix, float4(local_center, 1.0)).xyz;

    float3 view_vec = world_center - _frame_render_data.camera_position.xyz;
    float d = dot(view_vec, normalize(_frame_render_data.camera_lookat_d.xyz));

    if (d <= 0.001) return 0.0;

    float fx = _frame_render_data.projection_matrix[0][0] * 0.5 * (float)_frame_render_data.render_target_width;
    float fy = -1.0f * _frame_render_data.projection_matrix[1][1] * 0.5 * (float)_frame_render_data.render_target_height;

    float world_projected_area_avg = (S.x * S.y + S.y * S.z + S.z * S.x) * 0.5;
    float pixel_area = world_projected_area_avg * (fx * fy) / (d * d);

    return pixel_area;
}

bool is_soft_rasterize(
    Cluster cluster,
    float4x4 model_matrix,
    GPU_FrameRenderData _frame_render_data){
    
    float screen_pixel_area = CalculateAABBProjectedPixelsExact(
        float3(cluster.self_aabb[0], cluster.self_aabb[1], cluster.self_aabb[2]),
        float3(cluster.self_aabb[3], cluster.self_aabb[4], cluster.self_aabb[5]),
        model_matrix,
        _frame_render_data
    );

    if(screen_pixel_area < 0.0f) return false;

    return screen_pixel_area < float(cluster.triangle_count) * 3.0f;
}

// ============================================================================
// FIX: thread_idx はウェーブ内で単調増加、cluster_num_prefixsumも単調なので、
// ウェーブの最小/最大thread_idxに対してだけこの関数で二分探索し、
// その結果で各レーンの探索範囲を絞り込む(下のmain_comp内で使用)。
// ============================================================================
uint32_t BinarySearchPrefixSum(uint32_t key, uint32_t low, uint32_t high){
    while (low < high){
        uint32_t mid = (low + high) / 2;
        if (cluster_num_prefixsum[mid + 1] <= key){
            low = mid + 1;
        }else{
            high = mid;
        }
    }
    return low;
}

// グループ全体で合算するための共有メモリ
groupshared uint g_groupActiveCountHWRaster;
groupshared uint g_groupBaseIndexHWRaster;
groupshared uint g_groupActiveCountSWRaster;
groupshared uint g_groupBaseIndexSWRaster;

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
    
    // freeze culling
    if(frame_render_data.editor_data[4] == 1){
        return;
    }
    
    bool is_active = true;
    uint32_t thread_idx = global_thread_id.x;
    uint32_t total_cluster_num = cluster_num_prefixsum[frame_render_data.virtual_geometry_opaque_instance_ref_range.instance_ref_count];
    if(total_cluster_num <= global_thread_id.x){
        is_active = false;
        thread_idx = total_cluster_num - 1;
    }
    
    // -------------------------------------------------------------
    // FIX: 二分探索をウェーブレベルで絞り込む。
    // まずウェーブの最小/最大thread_idxに対して(全レーン一様・分岐なしで)
    // 二分探索し、答えが入り得る範囲[wave_low, wave_high]を求める。
    // 1インスタンスあたりのクラスタ数がウェーブ幅より十分大きければ、
    // 多くの場合 wave_low == wave_high (ウェーブ全体が1インスタンスに収まる)
    // となり、各レーンの本探索が実質0イテレーションで済む。
    //
    // 注意: 小さいクラスタ数のインスタンスが密集している場合、絞り込み用の
    // 2回の探索がほぼ無駄になり、素朴な二分探索より遅くなる可能性がある。
    // 実際のシーンデータで効果を確認することを推奨。
    // -------------------------------------------------------------
    uint32_t instance_ref_count = frame_render_data.virtual_geometry_opaque_instance_ref_range.instance_ref_count;
    uint32_t virtual_ref_idx = BinarySearchPrefixSum(thread_idx, 0, instance_ref_count);
    uint32_t cluster_num_sum = cluster_num_prefixsum[virtual_ref_idx];
    
    uint32_t cluster_id = thread_idx - cluster_num_sum;
    InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data.instance_reference_table[ref_range.instance_ref_base + virtual_ref_idx];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
    Cluster cluster = instance.cluster_addr[cluster_id];
    
    float threshold = 0.1;
    if(frame_render_data.editor_data[2] == 0){ // not auto
        if (cluster.lod_level != frame_render_data.editor_data[1]){
            is_active = false;
        }

    }else{ // auto lod selection
        
        // FIX: 対角成分ではなく各軸の長さを使う。
        // 対角成分は回転が入っていると純粋なスケールを表さないため、
        // 回転済みインスタンスでLODの誤差計算が狂うバグがあった。
        float scale = max(
            max(length(inst_ref.model_matrix[0].xyz), length(inst_ref.model_matrix[1].xyz)),
            length(inst_ref.model_matrix[2].xyz)
        );

        float4 child_lod_sphere = float4( cluster.self_lod_bound_sphere[0], cluster.self_lod_bound_sphere[1], cluster.self_lod_bound_sphere[2], cluster.self_lod_bound_sphere[3]);
        float child_error = lod_error_is_imperceptible(child_lod_sphere, cluster.error_self, inst_ref.model_matrix, scale);
        
        float4 parent_lod_sphere = float4(cluster.parent_lod_bound_sphere[0], cluster.parent_lod_bound_sphere[1], cluster.parent_lod_bound_sphere[2], cluster.parent_lod_bound_sphere[3]);
        float parent_error = lod_error_is_imperceptible(parent_lod_sphere, cluster.error_parent, inst_ref.model_matrix, scale);
        
        if(child_error <= threshold && threshold < parent_error){}
        else is_active = false;
    }
    
    // ==========================
    // culling
    // ==========================
    float3 aabb_min = float3(cluster.self_aabb[0], cluster.self_aabb[1], cluster.self_aabb[2]);
    float3 aabb_max = float3(cluster.self_aabb[3], cluster.self_aabb[4], cluster.self_aabb[5]);
    float3 bs_center = (aabb_min + aabb_max)/2.0;
    float bs_r = length(aabb_max - bs_center);

    uint2 screen_size = uint2(frame_render_data.render_target_width, frame_render_data.render_target_height);

    // FIX: frustum_culling と projection_bounding_sphere で同じMVPを
    // 個別に(2回)計算していたのを1回にまとめて使い回す。
    float4x4 mvp = mul(frame_render_data.projection_view_matrix, inst_ref.model_matrix);

    // frustum
    if(is_active){
        bool passed_frustum = frustum_culling(float4(bs_center, bs_r), mvp);
        is_active &= passed_frustum;
    }

    // FIX: projection_bounding_sphere / CoversPixelCenter を is_active でガード。
    // 元は既に is_active=false になったスレッドに対しても無条件で実行しており、
    // (LOD選択やfrustumカリングで)ウェーブ全体が非アクティブになるケースで
    // 無駄な計算(射影計算・近平面判定含む)が発生していた。
    ProjectionBoundingSphereInfo bs_info;
    if(is_active){
        bs_info = projection_bounding_sphere(
            float4(bs_center, bs_r),
            mvp,
            screen_size,
            0.00f
        );

        // micro cluster culling
        bool is_micro_cluster = CoversPixelCenter(bs_info);
        is_active &= is_micro_cluster;
    }

    // hiz
    bool passed_hiz = true;
    if(is_active){
        passed_hiz = hiz_occlusion_culling3(
            bs_info,
            screen_size,
            occlusion_hiz
        );
    }
    let is_false_negative = !passed_hiz;

    // recode
    // SWRaster, HWRaster の振り分け
    bool is_soft_raster = is_soft_rasterize(cluster, inst_ref.model_matrix, frame_render_data);
    if(frame_render_data.editor_data[3] == 1){ 
        is_soft_raster = true;
    }else if(frame_render_data.editor_data[3] == 2){
        is_soft_raster = false;
    }
    let is_hard_raster = !is_soft_raster;
    
    // -------------------------------------------------------------
    // 1. Wave内の集計
    // -------------------------------------------------------------
    uint waveActiveCountHWRaster = WaveActiveCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveLocalIndexHWRaster  = WavePrefixCountBits(is_active & passed_hiz & is_hard_raster);
    uint waveActiveCountSWRaster = WaveActiveCountBits(is_active & passed_hiz & is_soft_raster);
    uint waveLocalIndexSWRaster  = WavePrefixCountBits(is_active & passed_hiz & is_soft_raster);

    uint waveActiveCountHWRasterFN = WaveActiveCountBits(is_active & is_false_negative & is_hard_raster);
    uint waveLocalIndexHWRasterFN  = WavePrefixCountBits(is_active & is_false_negative & is_hard_raster);
    uint waveActiveCountSWRasterFN = WaveActiveCountBits(is_active & is_false_negative & is_soft_raster);
    uint waveLocalIndexSWRasterFN  = WavePrefixCountBits(is_active & is_false_negative & is_soft_raster);

    // 各Waveの先頭スレッド（Leader）が、グループ共有メモリにWaveごとのカウントを足し込む
    if (local_thread_id.x == 0)
    {
        g_groupActiveCountHWRaster = 0;
        g_groupActiveCountSWRaster = 0;
        g_groupActiveCountHWRasterFN = 0;
        g_groupActiveCountSWRasterFN = 0;
    }
    GroupMemoryBarrierWithGroupSync();

    uint waveBaseInGroupHWRaster = 0;
    uint waveBaseInGroupSWRaster = 0;
    uint waveBaseInGroupHWRasterFN = 0;
    uint waveBaseInGroupSWRasterFN = 0;

    if (WaveIsFirstLane())
    {
        InterlockedAdd(g_groupActiveCountHWRaster, waveActiveCountHWRaster, waveBaseInGroupHWRaster);
        InterlockedAdd(g_groupActiveCountSWRaster, waveActiveCountSWRaster, waveBaseInGroupSWRaster);
        InterlockedAdd(g_groupActiveCountHWRasterFN, waveActiveCountHWRasterFN, waveBaseInGroupHWRasterFN);
        InterlockedAdd(g_groupActiveCountSWRasterFN, waveActiveCountSWRasterFN, waveBaseInGroupSWRasterFN);
    }

    GroupMemoryBarrierWithGroupSync();

    waveBaseInGroupHWRaster = WaveReadLaneFirst(waveBaseInGroupHWRaster);
    waveBaseInGroupSWRaster = WaveReadLaneFirst(waveBaseInGroupSWRaster);
    waveBaseInGroupHWRasterFN = WaveReadLaneFirst(waveBaseInGroupHWRasterFN);
    waveBaseInGroupSWRasterFN = WaveReadLaneFirst(waveBaseInGroupSWRasterFN);

    // -------------------------------------------------------------
    // 2. グローバルAtomic Add（Thread 0 のみ実行）
    // -------------------------------------------------------------
    if (local_thread_id.x == 0)
    {
        InterlockedAdd(cluster_render_queue[0].head_num, g_groupActiveCountHWRaster, g_groupBaseIndexHWRaster);
        InterlockedAdd(cluster_render_queue[0].tail_num, g_groupActiveCountSWRaster, g_groupBaseIndexSWRaster);
        InterlockedAdd(cluster_render_queue[0].HW_fn_count, g_groupActiveCountHWRasterFN, g_groupBaseIndexHWRasterFN);
        InterlockedAdd(cluster_render_queue[0].SW_fn_count, g_groupActiveCountSWRasterFN, g_groupBaseIndexSWRasterFN);
        
        uint32_t tmp;
        InterlockedAdd(cluster_render_queue[0].fn_count, g_groupActiveCountHWRasterFN + g_groupActiveCountSWRasterFN, tmp);
        tmp = (tmp + CTA_NUM - 1) / CTA_NUM;
        InterlockedMax(cluster_render_queue[0].sx, tmp);
    }
    GroupMemoryBarrierWithGroupSync();

    // -------------------------------------------------------------
    // 3. 書き込み位置の計算と書き込み
    // -------------------------------------------------------------
    if(is_active & passed_hiz & is_hard_raster){
        uint writeIndex = g_groupBaseIndexHWRaster + waveBaseInGroupHWRaster + waveLocalIndexHWRaster;
        if(writeIndex < MAX_RENDER_CLUSTER){
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }

    if(is_active & passed_hiz & is_soft_raster){
        uint writeIndex = g_groupBaseIndexSWRaster + waveBaseInGroupSWRaster + waveLocalIndexSWRaster;
        if(writeIndex < MAX_RENDER_CLUSTER){
            writeIndex = MAX_RENDER_CLUSTER - 1 - writeIndex;
            cluster_render_queue[0].data[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }
    
    if(is_active & is_false_negative & is_hard_raster){
        uint writeIndex = g_groupBaseIndexHWRasterFN + waveBaseInGroupHWRasterFN + waveLocalIndexHWRasterFN;
        if(writeIndex < MAX_RENDER_CLUSTER){
            cluster_render_queue[0].data_false_negative[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }

    if(is_active & is_false_negative & is_soft_raster){
        uint writeIndex = g_groupBaseIndexSWRasterFN + waveBaseInGroupSWRasterFN + waveLocalIndexSWRasterFN;
        if(writeIndex < MAX_RENDER_CLUSTER){
            writeIndex = MAX_RENDER_CLUSTER - 1 - writeIndex;
            cluster_render_queue[0].data_false_negative[writeIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
        }
    }
}

