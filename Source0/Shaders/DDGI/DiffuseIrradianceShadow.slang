#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/SamplerType.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/DirectLighting.h.slang"
#include "../Include/ColorSpace.h.slang"

[[shader("raygen")]]
void main_rgen() {

    uint32_t2 screen_pos = DispatchRaysIndex().xy;
    
    float4 world_position = world_position_gb[screen_pos];

    if (world_position[3] == 1.0) {
        diffuse_irradiance_shadow_gb[screen_pos] = float4(0.0);
        return;
    }

    float3 world_normal = normal_occ_gb[screen_pos].rgb;
    world_normal = normalize(world_normal);
    
    // get irradiance from probe
    float3 irradiance = get_probe_irradiance(
        ddgi_desc,
        world_position.xyz,
        world_normal,
        normalize(world_position.xyz - frame_render_data.camera_position.xyz),
        probe_irradiance_texture,
        probe_chit_t_texture,
        samplers,
        probe_relocation_offset
    );
    
    // calc shadow
    float3 light_dir = frame_render_data.directional_light_dir.xyz;
    
    RayDesc ray_desc;
    ray_desc.Direction = light_dir;
    ray_desc.Origin = world_position.xyz + light_dir*1e-2;
    ray_desc.TMin = 1e-2;
    ray_desc.TMax = 1e5;
    
    RtPayload payload;
    TraceRay(accelaration_structures[0], RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);
    
    float light_visiblity = payload.is_hit ? 0.0 : 1.0;
    
    // store
    diffuse_irradiance_shadow_gb[screen_pos] = float4(irradiance, light_visiblity);
}

[[shader("miss")]]
void main_miss(inout RtPayload payload) {
    payload.is_hit = false;
}

[[shader("closesthit")]]
void main_chit(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload){
    payload.is_hit = true;
}

[[shader("anyhit")]]
void main_ahit(in BuiltInTriangleIntersectionAttributes attr) {

    RtPayload rt_payload;
    rt_payload.render_instance_id_base = InstanceID(); //cumtom がこっち
    rt_payload.render_instance_id = GeometryIndex();
    rt_payload.triangle_idx = PrimitiveIndex();
    rt_payload.hit_t = RayTCurrent();
    rt_payload.bary = attr.barycentrics;

    // instance
    //uint32_t instance_global_id = frame_render_data.rt_render_instance_global_id_map[rt_payload.render_instance_id_base + rt_payload.render_instance_id];
    Instance instance = frame_render_data.instance_table[rt_payload.render_instance_id_base + rt_payload.render_instance_id];

    // material
    Material *material = &frame_render_data.material_table[instance.material_id_base + instance.material_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;
    
    // barys
    float3 barys = float3((1.0 - rt_payload.bary.x - rt_payload.bary.y), rt_payload.bary.x, rt_payload.bary.y);
    
    // 
    VertexType0* raw_vertex_addr = instance.raw_vertex_addr;
    uint32_t* raw_index_addr = instance.raw_index_addr;

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
        AcceptHitAndEndSearch();
    }
}