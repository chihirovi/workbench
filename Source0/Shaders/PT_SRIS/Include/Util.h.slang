#pragma once
#include "../../Include/GPU_FrameRenderData.h.slang"

RayDesc create_first_ray(GPU_FrameRenderData render_data, uint2 screen_size, uint2 launch_id_xy){
    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    float3 w = -normalize(render_data.camera_lookat_d.xyz);
    float3 u = normalize(cross(render_data.camera_vup_vfov.xyz, w));
    float3 v = cross(w, u);
    
    // calc viewport
    float vfov = render_data.camera_vup_vfov[3];
    float focal_length = 5.0;
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
    float3 viewport_upper_left = render_data.camera_position.xyz
                         - focal_length*w
                         - viewport_u/2.0f 
                         - viewport_v/2.0f;

    float3 pixel00_loc = viewport_upper_left 
                + 0.5f * (pixel_delta_u + pixel_delta_v);  

    float3 target     = pixel00_loc.xyz 
                    + launch_id_xy.x*pixel_delta_u 
                    + launch_id_xy.y*pixel_delta_v;

    RayDesc ray;
    ray.Origin    = render_data.camera_position.xyz;
    ray.Direction = normalize(target - ray.Origin);
    ray.TMax = 1e5;
    ray.TMin = 1e-5;
    return ray;
}

Optional<float2> calc_ray_plane_intersection(
    float3 ray_O,
    float3 norm_ray_dir,
    float3 p_Q,
    float3 p_U,
    float3 p_V){
        
    float3 n = cross(p_U, p_V);
    float3 normal = normalize(n);
    float D = dot(normal, p_Q);
    float3 w = n / dot(n, n);

    float denom = dot(normal, norm_ray_dir);
    
    if(abs(denom) < 1e-5) return none;
    
    float t = (D - dot(normal, ray_O)) / denom;
    float3 intersection = ray_O + norm_ray_dir * t;
    float3 planar_hitpt = intersection - p_Q;
    float alpha = dot(w, cross(planar_hitpt, p_V));
    float beta = dot(w, cross(p_U, planar_hitpt));
    
    return float2(alpha, beta);
}

Optional<int32_t2> calc_pre_frame_screen_pos(
    GPU_FrameRenderData render_data, 
    RayDesc ray) {
    
    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    float3 w = -normalize(render_data.pre_camera_lookat_d.xyz);
    float3 u = normalize(cross(render_data.pre_camera_vup_vfov.xyz, w));
    float3 v = cross(w, u);

    // calc viewport
    float vfov = render_data.pre_camera_vup_vfov[3];
    float focal_length = 5.0;
    float aspect = (float)render_data.render_target_width / (float)render_data.render_target_height;
    float theta = (vfov) * 3.141592 / 180;
    float viewport_height = 2 * tan(theta / 2) * focal_length;
    float viewport_width = viewport_height * aspect;

    // Calculate the vectors across the horizontal and down the vertical viewport edges.
    float3 viewport_u = viewport_width* u;
    float3 viewport_v = viewport_height* -v;

    // Calculate the location of the upper left pixel.
    float3 viewport_upper_left = render_data.pre_camera_position.xyz
                         - focal_length*w
                         - viewport_u/2.0f 
                         - viewport_v/2.0f;

    Optional<float2> opt_pre_uv = calc_ray_plane_intersection(
        ray.Origin,
        ray.Direction,
        viewport_upper_left,
        viewport_u,
        viewport_v 
    );
    
    if(opt_pre_uv == none) return none;

    float2 pre_uv = opt_pre_uv.value;

    int32_t screen_x = int32_t(pre_uv.x * (float)render_data.render_target_width);
    int32_t screen_y = int32_t(pre_uv.y * (float)render_data.render_target_height);
    
    if((screen_x < 0) || (render_data.render_target_width <= screen_x)) return none;
    if((screen_y < 0) || (render_data.render_target_height <= screen_y)) return none;
    
    return int32_t2(screen_x, screen_y);
}