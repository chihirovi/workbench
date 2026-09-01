#pragma once
#include "Material.hpp"
#include "Instance.hpp"
#include "InstanceReference.hpp"
#include "InstanceRefRange.hpp"
#include "TextureView.hpp"
#include <vulkan/vulkan.hpp>
#include <vector>
#include <glm/glm.hpp>

namespace Irori::Render{
    
struct CPU_FrameRenderData{
public:
    // model data
    // 一つのモデルは複数の render instance (submesh) から構成される
    // 例えば，modelA = table[0..4], modelB table = [4..5], ...
    // みたいな感じ，
    // ASに関しては，このモデルごとに，BLAS があり，
    // BLAS のサブメッシュの並び順は table 内の サブメッシュの並びと一致している
    std::vector<Render::Instance> render_instance_table;

    std::vector<Render::InstanceReference> common_opaque_instance_refs;
    std::vector<Render::InstanceReference> common_mask_instance_refs;
    std::vector<Render::InstanceReference> common_blend_instance_refs;

    std::vector<Render::InstanceReference> virtual_geometry_opaque_instance_refs;
    std::vector<Render::InstanceReference> virtual_geometry_mask_instance_refs;
    std::vector<Render::InstanceReference> virtual_geometry_blend_instance_refs;
    
    std::vector<Render::Material>       material_table;
    std::vector<vk::ImageView>          texture_table;
    std::vector<Render::TextureView>    texture_view_table;
    
    // camera info  
    glm::f32vec4    camera_position;
    glm::f32vec4    camera_lookat_d;
    glm::f32vec4    camera_vup_vfov;
    glm::f32mat4x4  view_matrix;
    glm::f32mat4x4  projection_matrix;
    glm::f32mat4x4  projection_view_matrix;

    glm::f32vec4 pre_camera_position;
    glm::f32vec4 pre_camera_lookat_d;
    glm::f32vec4 pre_camera_vup_vfov;
    
    // light info
    glm::f32vec4    directional_light_rgba; //rgb:color, a:intensity
    glm::f32vec4    directional_light_dir; //rgb:dir
    glm::f32vec4    sky_light_rgba; //rgb:color, a:intensity
    
    // frame buffer size
    uint32_t render_target_width;
    uint32_t render_target_height;
    
    // ray trace
    bool is_need_to_rebuild_AS = false;
    std::vector<uint64_t> rt_lod1234_vert_idx_addr;

    // editor data
    uint32_t editor_data[16];

public:
    void reset(){
        // init, resize(0)で確保済みの領域はそのままなので，次が早い
        render_instance_table.resize(0);
        common_opaque_instance_refs.resize(0);
        common_mask_instance_refs.resize(0);
        common_blend_instance_refs.resize(0);
        virtual_geometry_opaque_instance_refs.resize(0);
        virtual_geometry_mask_instance_refs.resize(0);
        virtual_geometry_blend_instance_refs.resize(0);
        texture_table.resize(0);
        texture_view_table.resize(0);
        material_table.resize(0);       
        is_need_to_rebuild_AS = true;
        rt_lod1234_vert_idx_addr.resize(0);
    }
};

struct GPU_FrameRenderData{
    // Instance
    uint64_t instance_table;
    uint64_t instance_reference_table;
    
    // Materisl
    uint64_t material_table;
    
    // texture views
    uint64_t texture_view_table;

    // instance ranges
    InstanceRefRange common_opaque_instance_ref_range;
    InstanceRefRange common_mask_instance_ref_range;
    InstanceRefRange common_blend_instance_ref_range;
    InstanceRefRange virtual_geometry_opaque_instance_ref_range;
    InstanceRefRange virtual_geometry_mask_instance_ref_range;
    InstanceRefRange virtual_geometry_blend_instance_ref_range;
    
    // Camera info
    glm::f32vec4    camera_position;
    glm::f32vec4    camera_lookat_d;
    glm::f32vec4    camera_vup_vfov;
    glm::f32mat4x4  view_matrix;
    glm::f32mat4x4  projection_matrix;
    glm::f32mat4x4  projection_view_matrix;
    
    glm::f32vec4 pre_camera_position;
    glm::f32vec4 pre_camera_lookat_d;
    glm::f32vec4 pre_camera_vup_vfov;

    // light info    
    glm::f32vec4    directional_light_rgba; //rgb:color, a:intensity
    glm::f32vec4    directional_light_dir; //dir
    glm::f32vec4    sky_light_rgba; //rgb:color, a:intensity
    
    // frame buffer size
    uint32_t render_target_width;
    uint32_t render_target_height;
    
    // ray trace data
    uint64_t rt_lod1234_vert_idx_addr;

    // editor data
    uint32_t editor_data[16];

}__attribute__((packed, aligned(1)));

//static_assert(sizeof(GPU_FrameRenderData) == 368, "GPU_FrameRenderData must be 368 bytes!");

}