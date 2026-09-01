#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/Rt.h.slang"

[[vk_binding(0, 0)]]
RaytracingAccelerationStructure topLevelAS;

[[vk_binding(1, 0)]]
[[vk_image_format("rgba8")]]
RWTexture2D<float4> output_image;

struct Ray {
    float3 origin;
    float3 direction;
};

Ray create_first_ray(float3 camera_origin, float3 camera_lookat_d, uint2 screen_size, uint2 launch_id_xy){
    // Calculate the u,v,w unit basis vectors for the camera coordinate frame.
    float3 w = -normalize(camera_lookat_d);
    float3 u = normalize(cross(frame_render_data.camera_vup_vfov.xyz, w));
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
    float3 viewport_upper_left = camera_origin
                         - focal_length*w
                         - viewport_u/2.0f 
                         - viewport_v/2.0f;

    float3 pixel00_loc = viewport_upper_left 
                + 0.5f * (pixel_delta_u + pixel_delta_v);  

    float3 target     = pixel00_loc.xyz 
                    + launch_id_xy.x*pixel_delta_u 
                    + launch_id_xy.y*pixel_delta_v;

    Ray ray;
    ray.origin     = camera_origin;
    ray.direction = normalize(target - ray.origin);
    return ray;
}


[[shader("raygen")]]
void main_rgen(){
    uint3 launch_index = DispatchRaysIndex().xyz;
    uint3 launch_size = DispatchRaysDimensions().xyz;

    Ray ray = create_first_ray(
        frame_render_data.camera_position.xyz, 
        frame_render_data.camera_lookat_d.xyz, 
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        launch_index.xy
    );

    RayDesc rayDesc;
    rayDesc.Origin = ray.origin;
    rayDesc.Direction = ray.direction;
    rayDesc.TMin = 0.001;
    rayDesc.TMax = 10000.0;

    RtPayload payload;
    payload.is_hit = false;

    TraceRay(topLevelAS, RAY_FLAG_NONE, 0xFF, 0, 0, 0, rayDesc, payload);
    
    // miss
    if(payload.is_hit == false){
        output_image[launch_index.xy] = float4(0.0);
        return;
    }
    
    // hit
    VertexType0Attribute vertex_attribute = fetch_hit_point_info_linear_albedo_no_lod(
        rayDesc,
        0,
        payload,
        frame_render_data,
        textures,
        samplers
    );

    
    output_image[launch_index.xy].xyz = vertex_attribute.normal;
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
    payload.bary = attr.barycentrics;
    payload.blas_to_world_matrix = ObjectToWorld3x4();
}
