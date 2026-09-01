#pragma once
#include "../../Editor/EditorBase.hpp"
#include "../../Core/Common/Sampler.hpp"
#include "../../Scene/SceneManager.hpp"
#include <dearImgui/ImGuizmo.h>
#include <iostream>

namespace Irori::Engine::PathTrace{

class UniqueEditor : public Editor::UniqueEditorBase{
public:
    bool camera_reset = false;
    bool camera_lock = true;
    std::optional<std::string> selected_model_name;
    std::optional<std::string> selected_inst_name;
    bool is_using_gui= false;

    float frame_time_ms = 0.0;
    
    bool reset_pt_acc = false;
    uint32_t frame_conut = 0;

public:
    UniqueEditor(){;}

    UniqueEditor(Core::VulkanContext* vc, Core::UniqueSwapchain& swapchain)
        : UniqueEditorBase(vc, swapchain){
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
            
            if(ImGui::CollapsingHeader("Path Trace Info", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("%d Frame Count\n", frame_conut);
                reset_pt_acc = ImGui::Button("pt_Reset");
                if(reset_pt_acc) frame_conut = 0;
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