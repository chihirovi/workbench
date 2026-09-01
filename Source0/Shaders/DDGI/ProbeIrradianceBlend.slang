#include "../Include/GlobalDescriptor.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/ProbeBlend.h.slang"
#include "./Include/RayDirection.h.slang"


[shader("compute")]
[numthreads(DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER, DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER, 1)]
void main_comp(
    uint3 global_thread_id: SV_DispatchThreadID,
    uint3 local_thread_id: SV_GroupThreadID,
    uint3 group_id : SV_GroupID) {

    bool is_border_texel = (local_thread_id.x == 0 || local_thread_id.x == (DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER - 1)); // Border Columns
    is_border_texel |= (local_thread_id.y == 0 || local_thread_id.y == (DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER - 1));     // Border Rows



    uint32_t3 probe_index_3d = group_id;
    uint32_t probe_index_1d = probe_index_3D_to_1D(ddgi_desc, probe_index_3d);
    
    if(!is_border_texel){
        float2 norm_probe_octant_UV = get_normalized_octahedral_coordinates(
            local_thread_id.xy-1, 
            DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER-2
        );
        
        float3 texel_dir = get_direction_from_normalized_octahedral_coordinates(norm_probe_octant_UV);
        
        float3 radiance_acc         = float3(0.0);
        float cos_weight_acc        = 0.0;
        
        for(uint ray_idx=0; ray_idx < DDGI_PROBE_NUM_RAY; ray_idx++){

            float4 temp = probe_trace_results[probe_index_1d*DDGI_PROBE_NUM_RAY + ray_idx].rgba;
            float3 radiance = float3(temp[0], temp[1], temp[2]);
            // if(temp[3] <= 0.0) continue;
            if (temp[3] <= 0.0) {
                radiance = float3(0.0);
            }

            float3 ray_dir = get_probe_ray_direction(ddgi_desc, ray_idx);
            
            //sum radiance
            float cos_weight = max(0.0, dot(texel_dir, ray_dir));
            radiance_acc += radiance*cos_weight;
            cos_weight_acc += cos_weight;
        }

        //dst
        int32_t3 dst_texel = int32_t3(   
            local_thread_id.x + group_id.x*DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER,
            local_thread_id.y + group_id.y*DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER,
            group_id.z
        );

        //float hysteresis = 0.97;
        float hysteresis = 0.99;
        float epsilon = DDGI_PROBE_NUM_RAY*1e-5;

        //calc and store irradiance
        float3 irradiance = (2.0 * M_PI / (2.0 * max(cos_weight_acc, epsilon))) * radiance_acc;
        irradiance = pow(irradiance, 1.0/DDGI_IRRADIANCE_ENCODE_GAMMA);
        float3 old_irradiance = probe_irradiance_texture_general[dst_texel].rgb;
        probe_irradiance_texture_general[dst_texel].rgb = lerp(irradiance, old_irradiance, hysteresis);
        
        if(frame_render_data.editor_data[0] == 1){ //clear probe
            probe_irradiance_texture_general[dst_texel].rgb = float3(0.0);
        }
        
    }

    AllMemoryBarrierWithGroupSync();
    //GroupMemoryBarrierWithGroupSync();

    
    if(is_border_texel){
        update_border_texel<DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER>(probe_irradiance_texture_general, local_thread_id.xy, probe_index_3d);
    }
}