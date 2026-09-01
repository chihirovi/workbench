#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../DDGI/Include/FetchHitPointInfo.h.slang"
#include "Include/Util.h.slang"
#include "Include/Random.h.slang"
#include "Include/Lambert.h.slang"
#include "Include/PathTrace.h.slang"
#include "Include/PathTraceNEE.h.slang"
#include "Include/PathTraceMIS.h.slang"
#include "Include/AreaLight.h.slang"

[[vk_binding(0, 0)]]
[[vk_image_format("rgba32f")]]
RWTexture2D<float4> acc_image;

[[vk_binding(1, 0)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> output_image;

[[vk_binding(2, 0)]]
RWStructuredBuffer<float> light_tile_info;

[[shader("raygen")]]
void main_rgen(){
    uint3 launch_index = DispatchRaysIndex().xyz;
    uint3 launch_size = DispatchRaysDimensions().xyz;

    RayDesc ray_desc = create_first_ray(
        frame_render_data,
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        launch_index.xy
    );
    
    UniformRandom uniform_random = UniformRandom(frame_render_data.editor_data[1] + uint32_t(light_tile_info[0]), launch_size.x, launch_index.xy);
    
    AreaLightSampler area_light_sampler = AreaLightSampler(light_tile_info);

    //float3 radiance = path_trace(ray_desc, uniform_random, accelaration_structures[0], frame_render_data, textures, samplers);
    //float3 radiance = path_trace_nee(ray_desc, uniform_random, accelaration_structures[0], frame_render_data, textures, samplers, area_light_sampler);
    //float3 radiance = path_trace_mis(ray_desc, uniform_random, accelaration_structures[0], frame_render_data, textures, samplers, area_light_sampler);
    RtPayload payload;
    TraceRay(accelaration_structures[0], RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
    
    float3 result;

    if (payload.is_hit == false) {
        output_image[launch_index.xy].rgb = float3(0.0);

    } else {
        VertexType0Attribute hit_surface = fetch_hit_point_info_linear_albedo_no_lod(
            ray_desc,
            0,
            payload,
            frame_render_data,
            textures,
            samplers 
        );       
        output_image[launch_index.xy].rgb = hit_surface.normal;
    }


    // acc_image[launch_index.xy].rgb += radiance;
    // output_image[launch_index.xy].rgb = linear_to_srgb(acc_image[launch_index.xy].xyz/float(frame_render_data.editor_data[1]+1));
    
    if(frame_render_data.editor_data[0] == 1) acc_image[launch_index.xy] = float4(0.0f);
}

[[shader("miss")]]
void main_miss(inout RtPayload payload) {
    payload.is_hit = false;
}

[[shader("closesthit")]]
void main_chit(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload){
    payload.is_hit = true;
    payload.render_instance_id_base = InstanceID(); //cumtom がこっち
    payload.render_instance_id = GeometryIndex();
    payload.triangle_idx = PrimitiveIndex();
    payload.hit_t = RayTCurrent();
    payload.bary = attr.barycentrics;
    payload.blas_to_world_matrix = ObjectToWorld3x4();
}