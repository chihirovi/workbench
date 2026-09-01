#include "SceneManager.hpp"

namespace Irori::Scene{

UniqueSceneManager::UniqueSceneManager(Core::VulkanContext* vc){
    vulkan_context = vc;
    model_manager = UniqueModelManager(vulkan_context);
    dummy_texture = std::move(Core::UniqueImage::Builder2D(vulkan_context)
        .set_size_format_usage_layout(1,1,vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal)
        .build());
}

UniqueSceneManager& UniqueSceneManager::register_model(
    std::string model_name, 
    std::string gltf_file_path, 
    std::optional<std::string> ivg_file_name){

    model_manager.register_model(model_name, gltf_file_path, ivg_file_name);
    
    // set rebuild AS flag
    is_need_to_rebuild_AS = true;

    return *this;
}

UniqueSceneManager& UniqueSceneManager::instantiate(std::string model_inst_name, std::string model_name){
    // check
    ASSERT_WITH_MSG(model_manager.has_model_name(model_name), std::format("Object Manager doesn't have object {}, please register at first", model_name));
    ASSERT_TRUE_WITH_MSG((model_instance_name_to_model_name_table.contains(model_inst_name)), std::format("Scene Manager already has instance name : {}", model_inst_name));
    
    // register
    ModelInstance model_instance{
        .transform      = glm::f32mat4x4(1.0),
        .is_visible     = true,
    };

    model_instance_name_to_model_name_table[model_inst_name] = model_name;
    models_instances[model_name][model_inst_name] = model_instance;
    
    // set rebuild AS flag
    is_need_to_rebuild_AS = true;
    
    return *this;
}
    
UniqueSceneManager& UniqueSceneManager::set_transform(std::string model_inst_name, glm::f32mat4x4 transform){
    ASSERT_WITH_MSG((model_instance_name_to_model_name_table.contains(model_inst_name)), std::format("Scene Manager doesn't have instance name : {}", model_inst_name));

    auto& model_name = model_instance_name_to_model_name_table[model_inst_name];
    models_instances[model_name][model_inst_name].transform = transform;
    
    return *this;
}

UniqueSceneManager& UniqueSceneManager::apply_transform(std::string model_inst_name, glm::f32mat4x4 transform){
    ASSERT_WITH_MSG((model_instance_name_to_model_name_table.contains(model_inst_name)), std::format("Scene Manager doesn't have instance name : {}", model_inst_name));
    
    auto& model_name = model_instance_name_to_model_name_table[model_inst_name];
    auto& dst_transform = models_instances[model_name][model_inst_name].transform;
    dst_transform = dst_transform*transform;
    
    return *this;
}


UniqueSceneManager& UniqueSceneManager::set_visibility(std::string model_inst_name, bool is_visible){
    ASSERT_WITH_MSG((model_instance_name_to_model_name_table.contains(model_inst_name)), std::format("Scene Manager doesn't have instance name : {}", model_inst_name));

    auto& model_name = model_instance_name_to_model_name_table[model_inst_name];

    models_instances[model_name][model_inst_name].is_visible = is_visible;
}

void UniqueSceneManager::build_cpu_frame_render_data_sub(Render::CPU_FrameRenderData* cpu_frame_render_data, bool target_virtual_geometry){
     
    for(auto& [model_name, model_instances] : models_instances){

        // vg対象でvgがある，vgが対象でないかつvgがない, の2パターン以外でcontinue;
        bool has_vg = model_manager.has_virtual_geometry(model_name);
        if( !((target_virtual_geometry && has_vg) || (!target_virtual_geometry && !has_vg)) ) continue;
        
        // get textures vk image views
        auto& texture_table = cpu_frame_render_data->texture_table;
        uint32_t texture_id_base = texture_table.size(); 
        auto texture_imageviews = model_manager.get_texture_vkimageviews(model_name);
        texture_table.insert(texture_table.end(), texture_imageviews.begin(), texture_imageviews.end());
        
        // get texture views
        auto& texture_view_table = cpu_frame_render_data->texture_view_table;
        uint32_t texture_view_id_base = texture_view_table.size();
        auto texture_views = model_manager.get_texture_views(model_name);
        texture_view_table.insert(texture_view_table.end(), texture_views.begin(), texture_views.end());
        
        // get materials
        auto& material_table = cpu_frame_render_data->material_table;
        uint32_t material_id_base = material_table.size();
        auto render_materials = model_manager.get_render_materials(model_name, texture_id_base, texture_view_id_base);
        material_table.insert(material_table.end(), render_materials.begin(), render_materials.end());
        
        // get render instances
        auto render_instances_mesh_prim_ids = model_manager.get_render_instances_mesh_prim_ids(model_name, material_id_base, DEFAULT_MATERIAL_ID);

        auto render_instances = render_instances_mesh_prim_ids.first;

        render_inst_table_gltf_mesh_prim_id_map.insert(
            render_inst_table_gltf_mesh_prim_id_map.end(),
            render_instances_mesh_prim_ids.second.begin(),
            render_instances_mesh_prim_ids.second.end()
        );
        
        ASSERT_WITH_MSG((!model_name_to_render_inst_table.contains(model_name)), std::format("model_name_to_render_inst_table already contaits model name [{}]", model_name));

        model_name_to_render_inst_table[model_name] = {
            cpu_frame_render_data->render_instance_table.size(),  //base
            render_instances.size() //range
        };

        cpu_frame_render_data->render_instance_table.insert(
            cpu_frame_render_data->render_instance_table.end(),   
            render_instances.begin(),
            render_instances.end()
        );

        // render instance reference
        std::vector<Render::InstanceReference> render_instance_refs;

        render_instance_refs.reserve(render_instances.size());

        for(auto [render_inst_id, render_inst] : render_instances|std::views::enumerate){
            render_instance_refs.push_back(Render::InstanceReference{
                .render_instance_id_base = model_name_to_render_inst_table[model_name].first,
                .render_instance_id = uint32_t(render_inst_id),
                .model_matrix = render_inst.local_model_matrix,
            });
        }
        
        std::vector<Render::InstanceReference> tmp_opaque_instance_refs;
        std::vector<Render::InstanceReference> tmp_mask_instance_refs;
        std::vector<Render::InstanceReference> tmp_blend_instance_refs;

        for(auto& render_instance_ref : render_instance_refs){

            Render::Instance& render_instance = cpu_frame_render_data->render_instance_table[render_instance_ref.render_instance_id_base + render_instance_ref.render_instance_id];
            uint32_t blend_mode = material_table[render_instance.material_id_base + render_instance.material_id].blend_mode;
            
            if(blend_mode == Render::MaterialBlendMode::opaque){
                tmp_opaque_instance_refs.push_back(render_instance_ref);                   

            }else if(blend_mode == Render::MaterialBlendMode::mask){
                tmp_mask_instance_refs.push_back(render_instance_ref);

            }else if(blend_mode == Render::MaterialBlendMode::blend){
                tmp_blend_instance_refs.push_back(render_instance_ref);

            }else{
                ASSERT_WITH_MSG(false, "unknown material blend mode");          
            }
        }

        // create frame render data
        std::vector<Render::InstanceReference>* p_opaque_render_inst_refs   = nullptr;
        std::vector<Render::InstanceReference>* p_mask_render_inst_refs     = nullptr;
        std::vector<Render::InstanceReference>* p_blend_render_inst_refs    = nullptr;

        if(target_virtual_geometry){
            p_opaque_render_inst_refs   = &cpu_frame_render_data->virtual_geometry_opaque_instance_refs;
            p_mask_render_inst_refs     = &cpu_frame_render_data->virtual_geometry_mask_instance_refs;
            p_blend_render_inst_refs    = &cpu_frame_render_data->virtual_geometry_blend_instance_refs;

        }else{
            p_opaque_render_inst_refs   = &cpu_frame_render_data->common_opaque_instance_refs;
            p_mask_render_inst_refs     = &cpu_frame_render_data->common_mask_instance_refs;
            p_blend_render_inst_refs    = &cpu_frame_render_data->common_blend_instance_refs;
        }

        // さっき作った instance ref を複製する
        for(auto& [_ , model_instance] : model_instances){

            if(!model_instance.is_visible) continue;

            // opaque
            for(auto& opaque_inst_ref : tmp_opaque_instance_refs){
                auto& instance_refs = *p_opaque_render_inst_refs;
                instance_refs.push_back(opaque_inst_ref);
                instance_refs.back().model_matrix = model_instance.transform * instance_refs.back().model_matrix;
                instance_refs.back().reverse_transpose_3x3_model_matrix = glm::transpose(glm::inverse(glm::f32mat3x3(instance_refs.back().model_matrix)));
            }

            // mask
            for(auto& mask_inst_ref : tmp_mask_instance_refs){
                auto& instance_refs = *p_mask_render_inst_refs;
                instance_refs.push_back(mask_inst_ref);
                instance_refs.back().model_matrix = model_instance.transform * instance_refs.back().model_matrix;
                instance_refs.back().reverse_transpose_3x3_model_matrix = glm::transpose(glm::inverse(glm::f32mat3x3(instance_refs.back().model_matrix)));
            }

            // blend
            for(auto& blend_instance : tmp_blend_instance_refs){
                auto& instance_refs = *p_blend_render_inst_refs;
                instance_refs.push_back(blend_instance);
                instance_refs.back().model_matrix = model_instance.transform * instance_refs.back().model_matrix;
                instance_refs.back().reverse_transpose_3x3_model_matrix = glm::transpose(glm::inverse(glm::f32mat3x3(instance_refs.back().model_matrix)));
            }
        }
    }    
}

void UniqueSceneManager::build_cpu_frame_render_data(Render::CPU_FrameRenderData* cpu_frame_render_data){
    
    // これも初期化
    model_name_to_render_inst_table.clear();
    render_inst_table_gltf_mesh_prim_id_map.resize(0);
    
    // default material
    cpu_frame_render_data->material_table.push_back(UniqueModelManager::get_default_material());
    
    // dummy texture
    cpu_frame_render_data->texture_table.push_back(dummy_texture.image_view.get());
    
    // build common model render instance
    build_cpu_frame_render_data_sub(cpu_frame_render_data, false);

    // build virtual geometry model render instance
    build_cpu_frame_render_data_sub(cpu_frame_render_data, true);
    
    // camera
    cpu_frame_render_data->camera_position          = glm::f32vec4(camera.lookfrom, 1.0);
    cpu_frame_render_data->camera_lookat_d          = glm::f32vec4(camera.lookat_d, 1.0);
    cpu_frame_render_data->camera_vup_vfov          = glm::f32vec4(camera.vup, camera.fov);
    cpu_frame_render_data->view_matrix              = camera.view;
    cpu_frame_render_data->projection_matrix        = camera.projection;
    cpu_frame_render_data->projection_view_matrix   = camera.projection*camera.view;
    
    cpu_frame_render_data->pre_camera_position      = glm::f32vec4(camera.pre_lookfrom, 1.0);
    cpu_frame_render_data->pre_camera_lookat_d      = glm::f32vec4(camera.pre_lookat_d, 1.0);
    cpu_frame_render_data->pre_camera_vup_vfov      = glm::f32vec4(camera.pre_vup, camera.fov);
    
    // set RT AS rebuild flag
    cpu_frame_render_data->is_need_to_rebuild_AS = is_need_to_rebuild_AS;
    is_need_to_rebuild_AS = false;
    
    // light
    cpu_frame_render_data->directional_light_rgba = directional_light_rgba;
    cpu_frame_render_data->directional_light_dir = directional_light_dir;
    cpu_frame_render_data->sky_light_rgba = sky_light_rgba;
}

UniqueSceneManager& UniqueSceneManager::init_camera(uint32_t w, uint32_t h, glm::f32vec3 lookfrom, glm::f32vec3 lookat, float fov, glm::f32vec3 vup){
    camera = Camera(w, h, lookfrom, lookat, fov, vup);
    return *this;
}

UniqueSceneManager& UniqueSceneManager::update_camera(float time_delta, bool* key_response, float* mouse_offset){
    camera.update(time_delta, key_response, mouse_offset);   
    return *this;
}

UniqueSceneManager& UniqueSceneManager::set_directional_light(glm::f32vec4 light_rgba, glm::f32vec3 light_dir){
    directional_light_rgba = light_rgba;
    directional_light_dir = glm::f32vec4{glm::normalize(light_dir), 0.0f};
    return *this;
}

UniqueSceneManager& UniqueSceneManager::set_sky_light(glm::f32vec4 light_rgb_power){
    sky_light_rgba = light_rgb_power;
    return *this; 
}

}













