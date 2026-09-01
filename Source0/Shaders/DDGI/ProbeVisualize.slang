#include "./Include/Types.h.slang"
#include "./Include/ProbeIndex.h.slang"
#include "./Include/ProbeOctahedral.h.slang"
#include "./Include/ProbeTexture.h.slang"
#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/SamplerType.h.slang"
#include "../Include/ColorSpace.h.slang"

struct VSInput {
    [[vk::location(0)]]
    float3 position : POSITION;

    [[vk::location(1)]]
    float3 normal : NORMAL;
};

struct VSOutput{
    float4 position         : SV_Position;
    float3 normal           : NORMAL;
    uint32_t probe_id_1d    : PROBE_ID_1D;
};

[shader("vertex")]
VSOutput main_vert(in VSInput vs_input, in uint32_t instance_id : SV_InstanceID) {

    float4 pos = float4(vs_input.position, 1.0);

    pos.xyz *= 0.1;

    pos.xyz += probe_pos_from_index1D(ddgi_desc, instance_id, probe_relocation_offset);
    

    VSOutput vs_output;
    vs_output.position = mul(frame_render_data.projection_matrix, mul(frame_render_data.view_matrix, pos));
    vs_output.normal = vs_input.normal;
    vs_output.probe_id_1d = instance_id;
    return vs_output;
}

struct FSOutput {
    float4 albedo_out : SV_Target0;
};

[shader("fragment")]
FSOutput main_frag(in VSOutput vs_output) {

    uint32_t3 probe_id_3d = probe_index_1D_to_3D(ddgi_desc, vs_output.probe_id_1d);

    float3 probe_irradiance_texture_UV = get_probe_texture_UV<DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER>(
        ddgi_desc, 
        probe_id_3d, 
        vs_output.normal
    );

    float3 probe_irradiance = probe_irradiance_texture.SampleLevel(
        samplers[int(SAMPLER_TYPE::DDGI_SAMPLER)], 
        probe_irradiance_texture_UV, 0.0
    ).rgb;
    
    FSOutput fs_output;
    //fs_output.albedo_out = float4(linear_to_srgb(pow(probe_irradiance, DDGI_IRRADIANCE_ENCODE_GAMMA)), 1.0);
    fs_output.albedo_out = float4((pow(probe_irradiance, DDGI_IRRADIANCE_ENCODE_GAMMA)), 1.0);
    return fs_output;
}









