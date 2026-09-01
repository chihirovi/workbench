#include "../Include/GlobalDescriptor.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/ProbeBlend.h.slang"
#include "./Include/RayDirection.h.slang"


[shader("compute")]
[numthreads(DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER, DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER, 1)]
void main_comp(
    uint3 global_thread_id: SV_DispatchThreadID,
    uint3 local_thread_id: SV_GroupThreadID,
    uint3 group_id : SV_GroupID) {

    bool is_border_texel = (local_thread_id.x == 0 || local_thread_id.x == (DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER - 1)); // Border Columns
    is_border_texel |= (local_thread_id.y == 0 || local_thread_id.y == (DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER - 1));     // Border Rows

    uint32_t3 probe_index_3d = group_id;
    uint32_t probe_index_1d = probe_index_3D_to_1D(ddgi_desc, probe_index_3d);
    
    uint num_probes = ddgi_desc.sizes.x*ddgi_desc.sizes.y*ddgi_desc.sizes.z;
    
    if(!is_border_texel){
        float2 norm_probe_octant_UV = get_normalized_octahedral_coordinates(
            local_thread_id.xy-1, 
            DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER-2
        );
        
        float3 texel_dir = get_direction_from_normalized_octahedral_coordinates(norm_probe_octant_UV);
        
        float hit_t_acc             = 0.0;
        float hit_t2_acc            = 0.0;
        float cos_weight_acc        = 0.0;
        float cos_weight_pow_acc    = 0.0;
        
        float max_ray_distance = length(ddgi_desc.scales.xyz)*1.5f;
        for(uint ray_idx=0; ray_idx < DDGI_PROBE_NUM_RAY; ray_idx++){

            float4 temp = probe_trace_results[probe_index_1d*DDGI_PROBE_NUM_RAY + ray_idx].rgba;
            float hit_t = temp[3];
            // if(hit_t <= 0.0) continue;
            hit_t = min(abs(hit_t), max_ray_distance);
            hit_t = pow(hit_t, 50.0f);

            float3 ray_dir = get_probe_ray_direction(ddgi_desc, ray_idx);
            
            //sum hit_t
            float cos_weight = max(0.0, dot(texel_dir, ray_dir));    
            float cos_weight_pow = pow(cos_weight, 10.0);
            hit_t_acc += hit_t*cos_weight_pow;
            hit_t2_acc += hit_t*hit_t*cos_weight_pow;
            cos_weight_pow_acc += cos_weight_pow;
        }
        

        //dst
        int32_t3 dst_texel = int32_t3(   
            local_thread_id.x + group_id.x*DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER,
            local_thread_id.y + group_id.y*DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER,
            group_id.z
        );

        //float hysteresis = 0.97;
        float hysteresis = 0.99;
        float epsilon = DDGI_PROBE_NUM_RAY*1e-5;

        //calc and store distance average, variance
        float distance_ave  = hit_t_acc/max(cos_weight_pow_acc, epsilon);
        float distance2_ave = hit_t2_acc/max(cos_weight_pow_acc, epsilon);
        float2 store_dist = float2(distance_ave, distance2_ave);   
        float2 old_dist = probe_chit_t_texture_general[dst_texel].rg;
        probe_chit_t_texture_general[dst_texel].rg = lerp(store_dist, old_dist, hysteresis);

        if(frame_render_data.editor_data[0] == 1){ //clear probe
            probe_chit_t_texture_general[dst_texel].rg = float2(1e5);
        }
    }

    AllMemoryBarrierWithGroupSync();
    
    if(is_border_texel){
        update_border_texel<DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER>(probe_chit_t_texture_general, local_thread_id.xy, probe_index_3d);
    }
}