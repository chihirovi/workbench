#include "../Include/ColorSpace.h.slang"
#include "./Include/lighting.h.slang"
#include "../Include/GlobalDescriptor.h.slang"

[vk_binding(0, 0)]
[vk_image_format("rgba8")]
RWTexture2D<float4> output_image;


float calculate_luminance(float3 color){
    return dot(color, float3(0.299, 0.587, 0.114));
}

float3 tone_map_reinhard(float3 color) {
    float luminance = calculate_luminance(color);
    float reinhard = luminance / (1.0f + luminance);
    return color * (reinhard / luminance);
}

float3 pixel_dir(float3 camera_origin, float3 camera_lookat_d, float vfov, uint2 screen_size, uint2 launch_id_xy){
    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    float3 w = -normalize(camera_lookat_d);
    float3 u = normalize(cross(frame_render_data.camera_vup_vfov.xyz, w));
    float3 v = cross(w, u);
    
    // calc viewport
    float focal_length = 1.0;
    float aspect = (float)screen_size.x / (float)screen_size.y;
    float theta = (vfov) * 3.141592 / 180;
    float viewport_height = 2 * tan(theta / 2) * focal_length;
    float viewport_width = viewport_height * aspect;

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    float3 viewport_u = viewport_width* u;
    float3 viewport_v = viewport_height* -v;

    // Calculate the horizontal and vertical delta vectors from pixel to pixel.
    float3 pixel_delta_u = viewport_u / float(screen_size.x);
    float3 pixel_delta_v = viewport_v / float(screen_size.y);

    // Calculate the location of the upper left pixel.
    float3 viewport_upper_left = camera_origin
                         - focal_length*w
                         - viewport_u/2.0f 
                         - viewport_v/2.0f;

    float3 pixel00_loc = viewport_upper_left 
                + 0.5f * (pixel_delta_u + pixel_delta_v);  

    float3 target     = pixel00_loc.xyz 
                    + launch_id_xy.x*pixel_delta_u 
                    + launch_id_xy.y*pixel_delta_v;

    float3 direction = normalize(target - camera_origin);
    return direction;
}

[shader("compute")]
[numthreads(32, 32, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    uint2 screen_pos = global_thread_id.xy;
    if (screen_pos.x >= frame_render_data.render_target_width || screen_pos.y >= frame_render_data.render_target_height) return;
    
    float4 world_position = world_position_gb[screen_pos].xyzw;
    
    bool is_sky = false;
    if (world_position.w == 1.0) {
        float3 p_dir = pixel_dir(
            frame_render_data.camera_position.xyz, 
            frame_render_data.camera_lookat_d.xyz, 
            frame_render_data.camera_vup_vfov[3],
            uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
            screen_pos
        );

        float a = 0.5*(p_dir.y + 1.0);
        float3 bg_color = (1.0 - a) * float3(1.0, 1.0, 1.0) + a * float3(0.5, 0.7, 1.0);
        output_image[screen_pos] = float4(bg_color, 1.0);
        // output_image[screen_pos] = float4(0.0);
        is_sky = true;
    }

    float4 albedo = albedo_gb[screen_pos];
    float3 normal = normal_occ_gb[screen_pos].xyz;
    float occlusion = normal_occ_gb[screen_pos].w;
    float2 metal_rough = metal_rough_gb[screen_pos].xy;
    float metallicness = metal_rough.r;
    float roughness = clamp(metal_rough.g, 0.04, 1.0);
    float3 emissive = emissive_gb[screen_pos].xyz;
    float4 temp = diffuse_irradiance_shadow_gb[screen_pos];
    float3 diffuse_irradiance = temp.xyz;
    float light_visibility = temp.w;
    
    //float3 N = normalize(normal);
    float3 N = normalize(normal); //fp16の量子化でジャギーが出るので一応
    float3 V = normalize(frame_render_data.camera_position.xyz - world_position.xyz);
    float3 L = frame_render_data.directional_light_dir.xyz;
    
    // diffuse
    float3 diffuse_brdf = albedo.xyz/M_PI;
    float3 light_color = frame_render_data.directional_light_rgba.rgb;
    float light_vertical_irradiance = frame_render_data.directional_light_rgba.a;
    float NL = saturate(dot(N, L));

    float3 secondary_diffuse = diffuse_brdf * diffuse_irradiance;
    float3 direct_diffue = diffuse_brdf * light_color * light_vertical_irradiance * NL * light_visibility;
    float3 diffuse = direct_diffue + secondary_diffuse;
    //float3 diffuse = direct_diffue;

    // specular
    float3 specular = ggx_smith(albedo.rgb, metallicness, roughness, N, L, V, light_visibility) * (light_color * light_vertical_irradiance) * NL;
    // float3 specular = ggx_smith(albedo.rgb, metallicness, roughness, N, reflect(-V, N), V, 1.0) * 0.9 * NL;

    // total
    float3 color = (1.0 - metallicness) * diffuse + specular + emissive;
    float3 raw_radiance = color;

    // post process
    color = tone_map_reinhard(color);

    color = linear_to_srgb(color);
    if(!is_sky) output_image[screen_pos] = float4(color, 1.0);
    
     // screen shot
    // [3] : screen shot, None=0, tonemap_srgb=1, raw_radiance=2
    uint32_t screen_pos_1d = screen_pos.y*frame_render_data.render_target_width + screen_pos.x;

    if (frame_render_data.editor_data[3] == 1) {
        if(is_sky) color = float3(0.0);
        screen_shot_buffer[screen_pos_1d*3 + 0] = color.r;
        screen_shot_buffer[screen_pos_1d*3 + 1] = color.g;
        screen_shot_buffer[screen_pos_1d*3 + 2] = color.b;
    }
    if (frame_render_data.editor_data[3] == 2) {
        if(is_sky) raw_radiance = float3(0.0);
        screen_shot_buffer[screen_pos_1d*3 + 0] = raw_radiance.r;
        screen_shot_buffer[screen_pos_1d*3 + 1] = raw_radiance.g;
        screen_shot_buffer[screen_pos_1d*3 + 2] = raw_radiance.b;
    }
}
