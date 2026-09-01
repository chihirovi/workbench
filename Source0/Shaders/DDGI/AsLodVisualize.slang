#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/SamplerType.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/DirectLighting.h.slang"
#include "../DDGI/Include/FetchHitPointInfo.h.slang"
#include "../DDGI/Include/DiffuseBrdfForGI.h.slang"


[vk_binding(0, 0)]
[vk_image_format("rgba8")]
RWTexture2D<float4> output_image;

RayDesc create_first_ray(float3 camera_origin, float3 camera_lookat_d, float vfov, uint2 screen_size, uint2 launch_id_xy){
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

    RayDesc ray;
    ray.Origin = camera_origin;
    ray.Direction = normalize(target - ray.Origin);
    ray.TMin = 1e-5;
    ray.TMax = 1e5;
    return ray;
}

[[shader("raygen")]]
void main_rgen(){
    uint3 launch_index = DispatchRaysIndex().xyz;
    uint3 launch_size = DispatchRaysDimensions().xyz;

    RayDesc ray = create_first_ray(
        frame_render_data.camera_position.xyz, 
        frame_render_data.camera_lookat_d.xyz, 
        frame_render_data.camera_vup_vfov[3],
        uint2(frame_render_data.render_target_width, frame_render_data.render_target_height),
        launch_index.xy
    );
    
    uint8_t lod = uint8_t(frame_render_data.editor_data[1]);
    RtPayload payload;
    payload.is_hit = false;
    payload.as_lod = lod;
    TraceRay(accelaration_structures[lod], RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray, payload);

    if (payload.is_hit == false) return;
    
    // get hit info
    VertexType0Attribute vertex_attribute = fetch_hit_point_info_linear_albedo_lod(
        ray,
        lod,
        payload,
        frame_render_data,
        textures,
        samplers
    );
    
    //output_image[launch_index.xy].xyz = linear_to_srgb(vertex_attribute.albedo.xyz);
    output_image[launch_index.xy].xyz = linear_to_srgb(vertex_attribute.normal.xyz);
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
void main_ahit(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload_lod) {
    
    RtPayload rt_payload;
    rt_payload.render_instance_id_base = InstanceID(); //cumtom がこっち
    rt_payload.render_instance_id = GeometryIndex();
    rt_payload.triangle_idx = PrimitiveIndex();
    rt_payload.hit_t = RayTCurrent();
    rt_payload.bary = attr.barycentrics;
    rt_payload.blas_to_world_matrix = ObjectToWorld3x4();

    // instance
    Instance instance = frame_render_data.instance_table[rt_payload.render_instance_id_base + rt_payload.render_instance_id];

    // material
    Material *material = &frame_render_data.material_table[instance.material_id_base + instance.material_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;
    
    // barys
    float3 barys = float3((1.0 - rt_payload.bary.x - rt_payload.bary.y), rt_payload.bary.x, rt_payload.bary.y);
    
    // 
    VertexType0* raw_vertex_addr;
    uint32_t* raw_index_addr;
    get_as_lod_vertex_index_addr(uint8_t(payload_lod.as_lod), frame_render_data, rt_payload, raw_vertex_addr, raw_index_addr);

    // index
    uint32_t index0 = raw_index_addr[rt_payload.triangle_idx*3 + 0];
    uint32_t index1 = raw_index_addr[rt_payload.triangle_idx*3 + 1];
    uint32_t index2 = raw_index_addr[rt_payload.triangle_idx*3 + 2];
    
    // uv
    float2 uv0 = unpackHalf2x16ToFloat(raw_vertex_addr[index0].packed_tex_coord);
    float2 uv1 = unpackHalf2x16ToFloat(raw_vertex_addr[index1].packed_tex_coord);
    float2 uv2 = unpackHalf2x16ToFloat(raw_vertex_addr[index2].packed_tex_coord);
    float2 uv = barys[0]*uv0 + barys[1]*uv1 + barys[2]*uv2;
    
    //alpha
    float tex_alpha = fetch_texture_lod(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base,
        texture_id_base,
        material->texture_view_id_albedo,
        uv,
        ALBEDO_TEX_DEFAULT_VALUE,
        1.0
    ).a;

    // 
    if (tex_alpha < material.alpha_cut_off) {
        IgnoreHit();
    }else{
    }
}