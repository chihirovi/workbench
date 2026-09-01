#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"
#include "../Include/FetchTexture.h.slang"
#include "../Include/VertexUtility.h.slang"

#include "Include/LightSampleDesc.h.slang"
#include "Include/Rand.h.slang"

[shader("compute")]
[numthreads(64, 1, 1)]
void main_comp(
    in uint3 global_thread_id : SV_DispatchThreadID,
    in uint3 local_thread_id : SV_GroupThreadID) {
    
    uint32_t light_sample_index = global_thread_id.x;
    
    Random rand = Random(uint3(light_sample_index, frame_render_data.editor_data[14], 1));
    
    // select light object
    uint32_t i = uint32_t(rand.Get() * light_sample_desc.light_object_num);
    WA_Entry lo_wa_entry = light_sample_desc.light_object_pdf_wa_table[i];
    uint32_t light_object_index = (rand.Get() <= lo_wa_entry.threshold) ? i : lo_wa_entry.index;
    LightObject light_object = light_sample_desc.light_object_table[light_object_index];

    // select primitive
    uint32_t j = uint32_t(rand.Get() * light_object.primitive_wa_table_range.count);
    WA_Entry lp_wa_entry = light_sample_desc.light_primitive_pdf_wa_table[light_object.primitive_wa_table_range.base + j];
    uint32_t light_primitive_index = (rand.Get() <= lp_wa_entry.threshold) ? j : lp_wa_entry.index;
    
    // select light point
    float su = sqrt(rand.Get());
    float b = 1.0 - su;
    float c = rand.Get() * su;
    float a = 1.0 - b - c;
    //pos = a * vertex0 + b * vertex1 + c * vertex2;
    float3 barys = float3(a, b, c);
    
    // calc light point
    Instance instance = frame_render_data.instance_table[light_object.render_instance_base + light_object.render_instance_index];
    
    // material
    Material material = frame_render_data.material_table[instance.material_id_base + instance.material_id];
    uint32_t texture_id_base = material.texture_id_base;
    uint32_t texture_view_id_base = material.texture_view_id_base;
    
    // 
    VertexType0* raw_vertex_addr = instance.raw_vertex_addr;
    uint32_t* raw_index_addr = instance.raw_index_addr;

    // index
    uint32_t index0 = raw_index_addr[light_primitive_index*3 + 0];
    uint32_t index1 = raw_index_addr[light_primitive_index*3 + 1];
    uint32_t index2 = raw_index_addr[light_primitive_index*3 + 2];
    
    // uv
    float2 uv0 = unpackHalf2x16ToFloat(raw_vertex_addr[index0].packed_tex_coord);
    float2 uv1 = unpackHalf2x16ToFloat(raw_vertex_addr[index1].packed_tex_coord);
    float2 uv2 = unpackHalf2x16ToFloat(raw_vertex_addr[index2].packed_tex_coord);

    float2 uv = barys[0]*uv0 + barys[1]*uv1 + barys[2]*uv2;
    
    // position
    float4 v0 = float4(raw_vertex_addr[index0].position, 1.0);
    float4 v1 = float4(raw_vertex_addr[index1].position, 1.0);
    float4 v2 = float4(raw_vertex_addr[index2].position, 1.0);

    float4 lp0 = mul(instance.model_matrix, v0);
    float4 lp1 = mul(instance.model_matrix, v1);
    float4 lp2 = mul(instance.model_matrix, v2);
    float primitive_local_area = 0.5 * length(cross(lp1.xyz - lp0.xyz, lp2.xyz - lp0.xyz));

    float4 tmp_position = barys[0] * lp0 + barys[1] * lp1 + barys[2] * lp2;
    float3 position = mul(light_object.scene_model_transform, tmp_position).xyz;
    
    // normal // 法線マップをわざわざ使う必要はない気がする
    float3 vertex_normal0 = decode_spherical_16(raw_vertex_addr[index0].encode_normal);
    float3 vertex_normal1 = decode_spherical_16(raw_vertex_addr[index1].encode_normal);
    float3 vertex_normal2 = decode_spherical_16(raw_vertex_addr[index2].encode_normal);
    
    float3 normal = barys[0]*vertex_normal0 + barys[1]*vertex_normal1 + barys[2]*vertex_normal2;
    normal = normalize(mul(mul(float32_t3x3(light_object.scene_model_transform), float32_t3x3(instance.model_matrix)) , normal));
    
    // emissive
    float3 emissive = fetch_texture_lod(
        frame_render_data,
        textures,
        samplers,
        texture_view_id_base, 
        texture_id_base,
        material.texture_view_id_emissive, 
        uv,
        EMISSIVE_TEX_DEFAULT_VALUE,
        1.0
    ).rgb;
    emissive.xyz = srgb_to_linear(emissive.xyz);
    float3 emissive_factor = float3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);
    emissive = emissive.xyz*emissive_factor;

    // recode light sample
    LightSample light_sample;
    light_sample.position = position;
    light_sample.emittanse = emissive; 
    light_sample.normal = normal;
    // light_sample.inv_pdf = light_object.inv_pdf * (light_object.surface_total_area / primitive_area) * primitive_area;
    light_sample.inv_pdf = light_object.inv_pdf* light_object.surface_total_area; // 上が正しい，今回はたまたま約分できるので，こうした
    light_samples_4096[light_sample_index] = light_sample;
    return;
}
