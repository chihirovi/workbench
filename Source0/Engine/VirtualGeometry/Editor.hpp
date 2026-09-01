#pragma once
#include "../../Editor/EditorBase.hpp"
#include "../../Core/Common/Sampler.hpp"
#include "../../Scene/SceneManager.hpp"
#include <dearImgui/ImGuizmo.h>
#include <iostream>

namespace Irori::Engine::VirtualGeometry{

class UniqueEditor : public Editor::UniqueEditorBase{
public:
    bool camera_reset = false;
    bool camera_lock = false;
    std::optional<std::string> selected_model_name;
    std::optional<std::string> selected_inst_name;
    bool is_using_gui= false;

    float frame_time_ms = 0.0;
    
    uint32_t frame_conut = 0;

    uint32_t selected_mode = 3;

    int selected_lod = 0;
    bool is_auto = true;
    
    uint32_t raster_mode = 0;
    
    VkDescriptorSet descriptor_sets[12];
    vk::UniqueSampler sampler;
    int occlusion_hiz_level = 0;
    
    bool is_freeze_culling = false;

public:
    UniqueEditor(){;}

    UniqueEditor(Core::VulkanContext* vc, Core::UniqueSwapchain& swapchain)
        : UniqueEditorBase(vc, swapchain){
    }
    
    void add_occlusion_hiz(vk::ImageView* image_views){
        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info
            .setAnisotropyEnable(vk::False)
            .setMagFilter(vk::Filter::eNearest)
            .setMinFilter(vk::Filter::eNearest)
            .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
            .setAddressModeW(vk::SamplerAddressMode::eClampToEdge)
            .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
            .setUnnormalizedCoordinates(vk::False)
            .setCompareEnable(vk::False)
            .setCompareOp(vk::CompareOp::eAlways)
            .setMipmapMode(vk::SamplerMipmapMode::eNearest) 
            .setMipLodBias(0.0)
            .setMinLod(0.0)
            .setMaxLod(0.0);
        
        sampler = vulkan_context->device->createSamplerUnique(sampler_create_info);
        
        for(int i=0; i<=11; i++){
            descriptor_sets[i] = ImGui_ImplVulkan_AddTexture(
                sampler.get(),
                image_views[i],
                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
            );            
        }
    }
    
    void show_occlushion_hiz(){
        if(ImGui::Begin("occlusion hiz")){

            ImGui::SliderInt("Occlusion Hiz Level", &occlusion_hiz_level, 0, 11);
            ImGui::Text(" ");
            ImGui::Image((ImTextureID)(descriptor_sets[occlusion_hiz_level]), ImVec2(512, 512));
        }
        ImGui::End();
    }
    
    void cmd_render(vk::CommandBuffer& cmd_buf, Scene::UniqueSceneManager& scene_manager){
        cmd_render_(cmd_buf, [&](){
                
            frame_conut++;

            // window
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGui::Begin("Render Infos");

            if(ImGui::CollapsingHeader("Frame Render Time (ms)", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("%.03f ms\n", frame_time_ms);
                ImGui::Text(" ");
            }
            
            

            // camera
            if(ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)){
                auto lookfrom = scene_manager.camera.lookfrom;
                auto lookat = scene_manager.camera.lookat_d;
                ImGui::DragFloat3("Lookfrom", &lookfrom[0], 0.1f);
                ImGui::DragFloat3("Lookat", &lookat[0], 0.1f);
                camera_reset = ImGui::Button("Reset");
                ImGui::Checkbox("Camera Lock", &camera_lock);
                ImGui::Text(" ");
            }

            
            if(ImGui::CollapsingHeader("Debug View", ImGuiTreeNodeFlags_DefaultOpen)){
                const char* labels[6] = { "Triangle", "Cluster", "Material", "Albedo", "Lod" , "SW HW Raster"};
                for (int i = 0; i < 6; ++i) {
                    if (ImGui::RadioButton(labels[i], selected_mode == i))
                        selected_mode = i;
                    
                }
                ImGui::Text("\n");
                
                ImGui::Checkbox("Freeze Culling", &is_freeze_culling);
                ImGui::Text("\n");
            }
            
            if(ImGui::CollapsingHeader("Rasterize Mode", ImGuiTreeNodeFlags_DefaultOpen)){
                const char* labels[3] = { "Auto Raster", "Force SW Raster", "Force HW Raster"};
                for (int i = 0; i < 3; ++i) {
                    if (ImGui::RadioButton(labels[i], raster_mode == i))
                        raster_mode = i;
                }
                ImGui::Text("\n");
            }
            
            if(ImGui::CollapsingHeader("Lod Selection", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Checkbox("Auto", &is_auto);

                ImGui::SliderInt("Selected Lod", &selected_lod, 0, 16);

                ImGui::Text("Current value = %d", selected_lod);
            }

            ImGui::End();

            // object List
            ImGui::Begin("Object List");

            if(ImGui::Button("Select Reset")){
                selected_model_name = std::nullopt;
                selected_inst_name = std::nullopt;
            }
            ImGui::Text(" ");

            for(auto& [model_name, model_insts] : scene_manager.models_instances){
                ImGui::Text("%s", model_name.c_str());

                for(auto& [inst_name, inst]: model_insts){
                    std::string temp_name("    ");
                    temp_name += inst_name;
                    
                    bool is_selected = false;
                    if(model_name == selected_model_name && inst_name == selected_inst_name){
                        is_selected = true;
                    }

                    if(ImGui::Selectable(temp_name.c_str(), is_selected)){
                        selected_model_name = model_name;
                        selected_inst_name = inst_name;
                    }
                }
                ImGui::Text(" ");
            }

            ImGui::End();
            
            // occlusion hiz
            show_occlushion_hiz();

            if (ImGui::IsAnyItemActive() || ImGui::IsAnyItemHovered()) {
                // ユーザーがウィジェットを操作中、またはホバー中
                is_using_gui = true;
            }else{
                is_using_gui = false;
            }
        });
    }
};

}