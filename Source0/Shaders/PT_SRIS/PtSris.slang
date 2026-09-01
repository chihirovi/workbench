#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/Rt.h.slang"
#include "Include/Util.h.slang"

#include "Include/LightSampleDesc.h.slang"
#include "Include/ReservoirSample.h.slang"
#include "Include/PT_NeeSRIS.h.slang"
#include "Include/PT_NeeSRIS_TR.h.slang"
#include "Include/PT_Nee.h.slang"
#include "Include/PT.h.slang"
#include "Include/Rand.h.slang"

[[vk_binding(0, 0)]]
[[vk_image_format("rgba32f")]]
RWTexture2D<float4> acc_image;

[[vk_binding(1, 0)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> output_image;

[[vk_binding(2, 0)]]
RWStructuredBuffer<float3> image_export_buffer;

[[vk_binding(3, 0)]]
RWStructuredBuffer<Reservoir<ReservoirSample>, ScalarDataLayout> images_temporal_reservoirs[2];

float calculate_luminance(float3 color){
    return dot(color, float3(0.299, 0.587, 0.114));
}

float3 tone_map_reinhard(float3 color) {
    float luminance = calculate_luminance(color);
    float reinhard = luminance / (1.0f + luminance);
    return color * (reinhard / luminance);
}

[[shader("raygen")]]
void main_rgen(){

    static_assert(sizeof(ReservoirSample) == 68, "Reservoir sample Size Error.\n");
    static_assert(sizeof(Reservoir<ReservoirSample>) == 84, "Reservoir Size Error.\n");

    uint3 launch_index = DispatchRaysIndex().xyz;
    uint3 launch_size = DispatchRaysDimensions().xyz;
    
    RayDesc ray_desc = create_first_ray(
        frame_render_data,
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        launch_index.xy
    );
    
    uint32_t pixel_id_1d = launch_index.y * launch_size.x + launch_index.x;

    // 再投影
    Optional<int32_t2> opt_pre_screen_xy = calc_pre_frame_screen_pos(frame_render_data, ray_desc);

    Random rand = Random(uint3(pixel_id_1d, frame_render_data.editor_data[15], frame_render_data.editor_data[1]));

    if (frame_render_data.editor_data[1] == 0) {
        images_temporal_reservoirs[0][pixel_id_1d] = Reservoir<ReservoirSample>();
        images_temporal_reservoirs[1][pixel_id_1d] = Reservoir<ReservoirSample>();
    }

    float le_scale = 100;
    float3 radiance = float3(0.0);
    if (frame_render_data.editor_data[4] == 1) {
        radiance = path_trace_nee_sris_TR(
            ray_desc, 
            rand, 
            accelaration_structures[0], 
            frame_render_data, 
            textures, samplers, 
            le_scale, 
            images_temporal_reservoirs, 
            pixel_id_1d, 
            opt_pre_screen_xy, 
            frame_render_data.editor_data[1]
        );
    }else {
        radiance = path_trace_nee_sris(ray_desc, rand, accelaration_structures[0], frame_render_data, textures, samplers, le_scale);
    }
    //radiance = path_trace_nee(ray_desc, rand, accelaration_structures[0], frame_render_data, textures, samplers, le_scale);
    //float3 radiance = path_trace(ray_desc, rand, accelaration_structures[0], frame_render_data, textures, samplers, 100);
    //radiance = path_trace_nee_sris_TR(ray_desc, rand, accelaration_structures[0], frame_render_data, textures, samplers, le_scale, image_temporal_reservoir, pixel_id_1d);

    float3 linear_radiance;
    //if (true) { 
    if (false) { 
        acc_image[launch_index.xy].rgb += radiance;
        linear_radiance = acc_image[launch_index.xy].xyz/float(frame_render_data.editor_data[1]+1);
    }else{
        linear_radiance = radiance;
    }

    output_image[launch_index.xy].rgb = linear_to_srgb(tone_map_reinhard(linear_radiance));
    if(frame_render_data.editor_data[0] == 1) acc_image[launch_index.xy] = float4(0.0f);
    if(frame_render_data.editor_data[3] == 1){
        image_export_buffer[pixel_id_1d] = linear_radiance;
    }

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
[[shader("anyhit")]]
void main_ahit(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload) {
    IgnoreHit();
    // AcceptHitAndEndSearch();
    // ReportHit();
}