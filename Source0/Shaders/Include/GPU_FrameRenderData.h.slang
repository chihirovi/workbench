#pragma once
#include "Instance.h.slang"
#include "InstanceReference.h.slang"
#include "Material.h.slang"
#include "TextureView.h.slang"
#include "InstanceRefRange.h.slang"

struct GPU_FrameRenderData{
    // Instance
    Instance *instance_table;
    InstanceReference *instance_reference_table;
    
    // Materisl
    Material* material_table;
    
    // texture views
    TextureView* texture_view_table;
    
    // instance ranges
    InstanceRefRange common_opaque_instance_ref_range;
    InstanceRefRange common_mask_instance_ref_range;
    InstanceRefRange common_blend_instance_ref_range;
    InstanceRefRange virtual_geometry_opaque_instance_ref_range;
    InstanceRefRange virtual_geometry_mask_instance_ref_range;
    InstanceRefRange virtual_geometry_blend_instance_ref_range;
    
    // Camera info
    float4 camera_position;
    float4 camera_lookat_d;
    float4 camera_vup_vfov;
    float32_t4x4 view_matrix;
    float32_t4x4 projection_matrix;
    float32_t4x4 projection_view_matrix;

    float4 pre_camera_position;
    float4 pre_camera_lookat_d;
    float4 pre_camera_vup_vfov;
    
    // light info
    float4 directional_light_rgba; //rgb:color, a:intensity
    float4 directional_light_dir;  // rgb:dir
    float4 sky_light_rgba; //rgb:color, a:intensity

    // frame buffer size
    uint32_t render_target_width;
    uint32_t render_target_height;
    
    // ray trace data
    uint64_t* rt_lod1234_vert_idx_addr; //0番目には例外的に1ASあたりのストライドが入ってる
    
    // editor data
    // [0] : clear_probe
    // [1] : show lod n
    // [2] : lock lod
    // [3] : screen shot, None=0, tonemap_srgb=1, raw_radiance=2
    uint32_t editor_data[16]; 
};