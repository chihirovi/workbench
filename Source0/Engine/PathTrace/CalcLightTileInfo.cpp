#pragma once

#include "CalcLightTileInfo.hpp"


namespace Irori::Engine::PathTrace{

std::pair<std::vector<glm::f32vec3>, glm::f32vec4> CalcLightTileInfo(Irori::Scene::UniqueSceneManager& scene_manager, std::string model_inst_name, std::string file_name){
    // load gltf model
    tinygltf::Model gltf_model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;

    tinygltf::LoadImageDataFunction dummy_image_loader = [](
        tinygltf::Image* _0, const int _1, std::string* _2, std::string* _3, int _4, int _5, const unsigned char* _6, int _7, void* _8) -> bool {
        return true;
    };

    loader.SetImageLoader(dummy_image_loader, nullptr);
    ASSERT_WITH_MSG((loader.LoadASCIIFromFile(
        &gltf_model, 
        &err, 
        &warn, 
        file_name 
    )), std::format("load gltf file error {}", err));
    
    std::vector<Irori::Render::VertexType0> temp_verts;
    std::vector<uint32_t> temp_idxs;
    load_vertex_index(gltf_model, gltf_model.meshes[0].primitives[0], temp_verts, temp_idxs);
    

    std::vector<glm::f32vec3> verts;
    for(auto& i : temp_idxs){
        glm::f32vec3 pos = {temp_verts[i].position[0], temp_verts[i].position[1], temp_verts[i].position[2]};
        verts.push_back(pos);
    }
    

    auto& model_name = scene_manager.model_instance_name_to_model_name_table[model_inst_name];
    glm::f32mat4x4 ref_mat = scene_manager.models_instances[model_name][model_inst_name].transform;
    auto render_inst_data = scene_manager.model_manager.get_render_instances_mesh_prim_ids(model_name, 0, 0);
    glm::f32mat4x4 inst_mat = render_inst_data.first[0].local_model_matrix;
    auto mat = ref_mat*inst_mat;

    for(int i=0; i<verts.size(); i++){
        verts[i] = glm::f32vec3(mat*glm::f32vec4(verts[i], 1.0f));
    }
    
    // light power
    auto& material = gltf_model.materials[0];
    glm::f32vec4 light = {material.emissiveFactor[0], material.emissiveFactor[1], material.emissiveFactor[2], 1.0};
    if(material.extensions.contains("KHR_materials_emissive_strength")){
        if(material.extensions["KHR_materials_emissive_strength"].Has("emissiveStrength")){
            light[3] = (float)material.extensions["KHR_materials_emissive_strength"].Get("emissiveStrength").GetNumberAsDouble();
        }
    }
    
    return {verts, light};
}


}



