#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/SamplerType.h.slang"
#include "./Include/Utility.h.slang"
#include "./Include/Types.h.slang"
#include "./Include/DirectLighting.h.slang"
#include "../DDGI/Include/FetchHitPointInfo.h.slang"
#include "../DDGI/Include/DiffuseBrdfForGI.h.slang"

[[shader("raygen")]]
void main_rgen_probe_trace() {
    uint probe_ray_index = DispatchRaysIndex().x;
    uint probe_id_1d = DispatchRaysIndex().y;
    RayDesc ray_desc = create_probe_ray(ddgi_desc, probe_id_1d, probe_ray_index, probe_relocation_offset);
    
    // trace ray from probe
    uint8_t lod = 0;
    RtPayload payload;
    if (frame_render_data.editor_data[2] == 5) {
        float lod_offset = 3.0;
        lod = calc_lod(frame_render_data.camera_position.xyz, ray_desc.Origin, lod_offset);
    }else{
        lod = uint8_t(frame_render_data.editor_data[2]);
    }
    payload.as_lod = lod;
    TraceRay(accelaration_structures[NonUniformResourceIndex(lod)], RAY_FLAG_NONE, 0xFF, 0, 0, 0, ray_desc, payload);

    // miss : early return
    if(payload.is_hit == false){
        float3 sky_radiance = frame_render_data.sky_light_rgba.rgb * frame_render_data.sky_light_rgba.a; //radiance W*Sr-1*M-2
        recode_probe_trace_result(probe_trace_results, probe_id_1d, probe_ray_index, sky_radiance, 1e10);
        return ;
    }

    // get hit info
    VertexType0Attribute vertex_attribute = fetch_hit_point_info_linear_albedo_lod(
        ray_desc,
        lod,
        payload,
        frame_render_data,
        textures,
        samplers
    );


    // back face check
    bool is_back_face_hit = dot(-ray_desc.Direction, vertex_attribute.normal) <= 0.0;
    if(is_back_face_hit){

        if(vertex_attribute.is_double_siced){
            vertex_attribute.normal *= -1.0;
        }else{
            recode_probe_trace_result(probe_trace_results, probe_id_1d, probe_ray_index, float3(0.0), -payload.hit_t);
            return;
        }
    }

    // get irradiance from probe
    float3 irradiance = get_probe_irradiance(
        ddgi_desc,
        vertex_attribute.position,
        vertex_attribute.normal,
        ray_desc.Direction,
        probe_irradiance_texture,
        probe_chit_t_texture,
        samplers,
        probe_relocation_offset
    );

    float3 secondary_diffuse = min(vertex_attribute.albedo.rgb, float3(0.999))/M_PI*irradiance;

    // get direct_diffuse light
    float3 direct_diffuse = direct_diffuse_lighting(
        vertex_attribute.position,
        vertex_attribute.normal,
        vertex_attribute.albedo.rgb,
        uint8_t(payload.as_lod),
        accelaration_structures[NonUniformResourceIndex(payload.as_lod)],
        frame_render_data
    );

    float3 radiance = direct_diffuse + secondary_diffuse + vertex_attribute.emissive.rgb;
    recode_probe_trace_result(probe_trace_results, probe_id_1d, probe_ray_index, radiance, payload.hit_t);
}

[[shader("miss")]]
void main_miss_probe_trace(inout RtPayload payload) {
    payload.is_hit = false;
}

[[shader("closesthit")]]
void main_chit_probe_trace(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload){
    payload.is_hit = true;
    payload.render_instance_id_base = InstanceID(); //cumtom がこっち
    payload.render_instance_id = GeometryIndex();
    payload.triangle_idx = PrimitiveIndex();
    payload.hit_t = RayTCurrent();
    payload.bary = attr.barycentrics;
    payload.blas_to_world_matrix = ObjectToWorld3x4();
}


[[shader("anyhit")]]
void main_ahit_probe_trace(in BuiltInTriangleIntersectionAttributes attr, inout RtPayload payload_lod) {

    payload_lod.render_instance_id_base = InstanceID(); //cumtom がこっち
    payload_lod.render_instance_id = GeometryIndex();
    payload_lod.triangle_idx = PrimitiveIndex();
    payload_lod.hit_t = RayTCurrent();
    payload_lod.bary = attr.barycentrics;
    payload_lod.blas_to_world_matrix = ObjectToWorld3x4();

    // instance
    Instance instance = frame_render_data.instance_table[payload_lod.render_instance_id_base + payload_lod.render_instance_id];

    // material
    Material *material = &frame_render_data.material_table[instance.material_id_base + instance.material_id];
    uint32_t texture_id_base = material->texture_id_base;
    uint32_t texture_view_id_base = material->texture_view_id_base;

    // barys
    float3 barys = float3((1.0 - payload_lod.bary.x - payload_lod.bary.y), payload_lod.bary.x, payload_lod.bary.y);
    
    // 
    VertexType0* raw_vertex_addr;
    uint32_t* raw_index_addr;
    get_as_lod_vertex_index_addr(uint8_t(payload_lod.as_lod), frame_render_data, payload_lod, raw_vertex_addr, raw_index_addr);

    // index
    uint32_t index0 = raw_index_addr[payload_lod.triangle_idx*3 + 0];
    uint32_t index1 = raw_index_addr[payload_lod.triangle_idx*3 + 1];
    uint32_t index2 = raw_index_addr[payload_lod.triangle_idx*3 + 2];
    
    // uv
    float2 uv0 = unpackHalf2x16ToFloat(raw_vertex_addr[index0].packed_tex_coord);
    float2 uv1 = unpackHalf2x16ToFloat(raw_vertex_addr[index1].packed_tex_coord);
    float2 uv2 = unpackHalf2x16ToFloat(raw_vertex_addr[index2].packed_tex_coord);
    float2 uv = barys[0] * uv0 + barys[1] * uv1 + barys[2] * uv2;

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
    if (tex_alpha < float(material->alpha_cut_off)) { //NG
        IgnoreHit();
    }
    return;
}