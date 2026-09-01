#pragma once
#include "../../Include/GPU_FrameRenderData.h.slang"

RayDesc create_first_ray(GPU_FrameRenderData render_data, uint2 screen_size, uint2 launch_id_xy){
    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    float3 w = -normalize(render_data.camera_lookat_d.xyz);
    float3 u = normalize(cross(render_data.camera_vup_vfov.xyz, w));
    float3 v = cross(w, u);
    
    // calc viewport
    float vfov = 40.0;
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
    ray.Origin      = render_data.camera_position.xyz;
    ray.Direction = normalize(target - ray.Origin);
    ray.TMax = 1e5;
    ray.TMin = 1e-5;
    return ray;
}