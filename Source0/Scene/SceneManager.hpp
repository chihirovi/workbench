#pragma once
#include "ModelManager.hpp"
#include "Camera.hpp"
#include "../Render/FrameRenderData.hpp"

#define MAX_RENDER_INSTANCE_NUM     (65536*128)

namespace Irori::Scene{

// -------------------------------------
struct ModelInstance{
    glm::f32mat4x4 transform;
    bool is_visible;
};


// -------------------------------------
class UniqueSceneManager{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSceneManager);

public:
    constexpr static int            DEFAULT_MATERIAL_ID = 0;
    Core::VulkanContext*            vulkan_context = nullptr;
    UniqueModelManager              model_manager;
    Camera                          camera;

    // model_name -> model_instance のマッピング
    std::unordered_map<std::string, std::unordered_map<std::string, ModelInstance>> models_instances; 
    
    // model_name -> render_instance_table{base, num} のマッピング
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> model_name_to_render_inst_table;
    
    // {mesh id, primitie id}
    // 純粋に，cpu_frame_render_dataのrender_intance_tableの各エントリーが，元のgltf fileでのmesh prim idを記録
    std::vector<std::pair<uint32_t, uint32_t>> render_inst_table_gltf_mesh_prim_id_map;
    
    // model instance name -> model name，のマッピング
    std::unordered_map<std::string, std::string> model_instance_name_to_model_name_table;
    
    // directional light
    glm::f32vec4 directional_light_rgba;
    glm::f32vec4 directional_light_dir;
    glm::f32vec4 sky_light_rgba; //rgb:color, a:intensity
                                    
    // for RT
    // 毎フレームごとに更新される
    bool is_need_to_rebuild_AS = false;

    // dummy texture
    Core::UniqueImage dummy_texture;
    
public:
    UniqueSceneManager(){;}
    UniqueSceneManager(Core::VulkanContext* vc);
    UniqueSceneManager(BOOST_RV_REF(UniqueSceneManager) rhs) = default;               //move constractor
    UniqueSceneManager& operator=(BOOST_RV_REF(UniqueSceneManager) rhs) = default;    //move assignment
                                                                                      
    UniqueSceneManager& register_model(std::string model_name, std::string gltf_file_path, std::optional<std::string> ivg_file_name);
    UniqueSceneManager& instantiate(std::string model_inst_name, std::string model_name);
    UniqueSceneManager& init_camera(uint32_t w, uint32_t h, glm::f32vec3 lookfrom, glm::f32vec3 lookat, float fov, glm::f32vec3 vup);
    UniqueSceneManager& update_camera(float time_delta, bool* key_response, float* mouse_offset);
    
    UniqueSceneManager& set_transform(std::string model_inst_name, glm::f32mat4x4 transform);
    UniqueSceneManager& apply_transform(std::string model_inst_name, glm::f32mat4x4 transform);

    UniqueSceneManager& set_visibility(std::string model_inst_name, bool is_visible);
    
    
    UniqueSceneManager& set_directional_light(glm::f32vec4 light_rgba, glm::f32vec3 light_dir);
    UniqueSceneManager& set_sky_light(glm::f32vec4 light_rgb_power);
    
    void build_cpu_frame_render_data_sub(Render::CPU_FrameRenderData* cpu_frame_render_data, bool target_virtual_geometry);
    void build_cpu_frame_render_data(Render::CPU_FrameRenderData* cpu_frame_render_data);
};
    
}














