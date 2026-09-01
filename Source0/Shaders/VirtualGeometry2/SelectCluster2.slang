#include "Include/GlobalDEscriptor.h.slang"
#include "../Include/GlobalDEscriptor.h.slang"
#include "Include/VG2.h.slang"

/*
// こっちでは距離の二乗で割ってる
auto ErrorProjection = [&](const DirectX::XMFLOAT3& center, float error)
{
    float yScale = 1.0f / tanf(InFovYRad * 0.5f);
    float d2 = (center.x * center.x) + (center.y * center.y) + (center.z * center.z);
    return InScreenHeight * 0.5f * yScale * error / (d2 - error * error);
};
*/

float calc_error(float3 center, float error, float4x4 world_from_local, float world_scale) {
    float yScale = frame_render_data.projection_matrix[1][1] * 0.5f;
    float d2 = (center.x * center.x) + (center.y * center.y) + (center.z * center.z);
    return frame_render_data.render_target_height * 0.5f * yScale * error / (d2 - error * error) * world_scale;
}

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
    //projected_error /= distance_to_closest_point_on_sphere_clamped_to_znear;
    projected_error /= (distance_to_closest_point_on_sphere_clamped_to_znear * distance_to_closest_point_on_sphere_clamped_to_znear);

    projected_error *= -frame_render_data.projection_matrix[1][1] * 0.5;
    projected_error *= frame_render_data.render_target_height;

    return projected_error;
}

[shader("compute")]
[numthreads(32, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    uint32_t virtual_ref_idx = 0;
    uint32_t cluster_num_sum = 0;
    for(uint32_t i=0; i<1024; i++){

        if (cluster_num_prefixsum[i+1] == 0) return;

        if (cluster_num_prefixsum[i] <= global_thread_id.x && global_thread_id.x<cluster_num_prefixsum[i+1]) {
            cluster_num_sum = cluster_num_prefixsum[i];
            virtual_ref_idx = i;
            break;
        }
    }
    
    //
    uint32_t cluster_id = global_thread_id.x - cluster_num_sum;
    InstanceRefRange ref_range = frame_render_data.virtual_geometry_opaque_instance_ref_range;
    InstanceReference inst_ref = frame_render_data .instance_reference_table[ref_range.instance_ref_base + virtual_ref_idx];
    Instance instance = frame_render_data.instance_table[inst_ref.render_instance_id_base + inst_ref.render_instance_id];
    ClusterVG2 cluster = ((ClusterVG2*)instance.cluster_addr)[cluster_id];
    
    float threshold = 1.0;
    if(frame_render_data.editor_data[2] == 0){ // not auto
        if (cluster.lod != frame_render_data.editor_data[1]) return;

    }else{ // auto lod selection
        
        float scale = max(max(inst_ref.model_matrix[0][0], inst_ref.model_matrix[1][1]), inst_ref.model_matrix[2][2]);

        float4 child_lod_err_center = float4( cluster.self_err_bounds[0], cluster.self_err_bounds[1], cluster.self_err_bounds[2], cluster.self_err_bounds[3]);
        float child_error = lod_error_is_imperceptible(child_lod_err_center, cluster.self_err, inst_ref.model_matrix, scale);
        
        float4 parent_lod_err_center = float4(cluster.parent_err_bounds[0], cluster.parent_err_bounds[1], cluster.parent_err_bounds[2], cluster.parent_err_bounds[3]);
        float parent_error = lod_error_is_imperceptible(parent_lod_err_center, cluster.parent_err, inst_ref.model_matrix, scale);
        
        //if(child_error <= 0.3 && 0.3 < parent_error){}
        if(child_error <= threshold && threshold < parent_error){}
        else return;
    }
    /*
    // recode
    uint32_t org_value;
    InterlockedAdd(cluster_render_queue[0].head_num, 1, org_value);
    cluster_render_queue[0].data[org_value] = ((uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id));
    */
    // Wave内でアクティブなLane数
    uint activeCount = WaveActiveCountBits(true);

    // LeaderだけがAtomic
    uint baseIndex = 0;
    if (WaveIsFirstLane())
    {
        InterlockedAdd(cluster_render_queue[0].head_num, activeCount, baseIndex);
    }

    // 全Laneへ共有
    baseIndex = WaveReadLaneFirst(baseIndex);

    // Wave内での自分の書き込み位置
    uint localIndex = WavePrefixCountBits(true);

    // 書き込み
    cluster_render_queue[0].data[baseIndex + localIndex] = (uint64_t(virtual_ref_idx) << 32) | uint64_t(cluster_id);
}
    
