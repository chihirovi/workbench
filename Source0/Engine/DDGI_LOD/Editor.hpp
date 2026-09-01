#pragma once
#include "../../Editor/EditorBase.hpp"
#include "../../Core/Common/Sampler.hpp"
#include "../../Scene/SceneManager.hpp"
#include "../../DDGI/DdgiDesc.hpp"
#include <dearImgui/ImGuizmo.h>
#include <iostream>

namespace Irori::Engine::DDGI_LOD{

class UniqueEditor : public Editor::UniqueEditorBase{
public:
    bool camera_reset = false;
    bool camera_lock = false;
    std::optional<std::string> selected_model_name;
    std::optional<std::string> selected_inst_name;
    bool show_probe = false;
    bool show_lod = false;
    uint32_t lod = 0;
    uint32_t selected_lod = 5;
    uint32_t selected_lock_lod = 5;
    bool clear_probe = false;
    bool is_using_guizmo = false;
    bool is_using_gui= false;

    uint32_t selected_render_method = 0; //0=vertex, 1=mesh;
    bool use_mesh = false;

    float frame_time_ms = 0.0;
    
    double probe_trace_time = 0.0;
    double probe_trace_time_total = 0.0;
    uint32_t probe_trace_time_num = 0;

    bool screen_shot_tonemap_srgb = false;
    bool screen_shot_raw_radiance = false;
    bool export_screen_shot_tonemap_srgb = false;
    bool export_screen_shot_raw_radiance = false;
    
    uint32_t lod0_as_size=0;
    uint32_t lod1234_as_size=0;
    
    bool is_transpose = false;
public:
    UniqueEditor(){;}

    UniqueEditor(Core::VulkanContext* vc, Core::UniqueSwapchain& swapchain)
        : UniqueEditorBase(vc, swapchain){
    }
    
    void cmd_render(vk::CommandBuffer& cmd_buf, Scene::UniqueSceneManager& scene_manager, DDGI::DdgiDesc& ddgi_desc){
        cmd_render_(cmd_buf, [&](){

            // window
            ImGui::SetNextWindowBgAlpha(0.85f);
            ImGui::Begin("Render Infos");

            if(ImGui::CollapsingHeader("Frame Render Time (ms)", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("%.03f ms\n", frame_time_ms);
                ImGui::Text(" ");
            }

            // render method
            if(ImGui::CollapsingHeader("Render Method", ImGuiTreeNodeFlags_DefaultOpen)){

                const char* labels[2] = {"Vertex Shader", "Mesh shader"};

                for (int i = 0; i < 2; ++i) {
                    if (ImGui::RadioButton(labels[i], selected_render_method == i))
                        selected_render_method = i;
                    ImGui::SameLine(); // 横並びにしたい場合
                }
                if(selected_render_method == 0){
                    use_mesh = false;
                }else{
                    use_mesh = true;
                }

                ImGui::Text(" ");
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
            
            // ddgi
            if(ImGui::CollapsingHeader("DDGI", ImGuiTreeNodeFlags_DefaultOpen)){
                auto scales = ddgi_desc.gpu_desc.scales;
                glm::i32vec4 sizes = ddgi_desc.gpu_desc.sizes;
                int num = sizes.x*sizes.y*sizes.z;
                int total_ray = num*ddgi_desc.probe_num_ray;
                float view_bias = ddgi_desc.gpu_desc.view_normal_bias[0];
                float normal_bias = ddgi_desc.gpu_desc.view_normal_bias[1];

                ImGui::DragFloat("View Bias", &view_bias, 0.1f);
                ImGui::DragFloat("Normal Bias", &normal_bias, 0.1f);
                ImGui::DragFloat3("Probe Scales", &scales[0], 0.1f);
                ImGui::DragInt3("Probe Num", &sizes[0], 0.1f);
                ImGui::DragInt("Probe Total Num", &num, 0.1f);
                ImGui::DragInt("Probe Ray Total Num", &total_ray, 0.1f);
                ImGui::Text(" ");

                ImGui::Checkbox("Show Probe", &show_probe);
                ImGui::Text(" ");

                clear_probe = ImGui::Button("Clear Probe");
                ImGui::Text(" ");
                
                const char* labels[6] = { "Lod0", "Lod1", "Lod2", "Lod3", "Lod4", "Disable" };

                ImGui::Text("Show AS LOD:");
                for (int i = 0; i < 6; ++i) {
                    if (ImGui::RadioButton(labels[i], selected_lod == i))
                        selected_lod = i;
                    ImGui::SameLine(); // 横並びにしたい場合
                }
                if(selected_lod == 5){
                    show_lod = false;
                }else{
                    show_lod = true;
                    lod = selected_lod;
                }
                ImGui::Text(" ");

                ImGui::Text("lock AS LOD:");
                const char* labels1[6] = { "0", "1", "2", "3", "4", "Auto" };
                for (int i = 0; i < 6; ++i) {
                    if (ImGui::RadioButton(labels1[i], selected_lock_lod == i))
                        selected_lock_lod = i;
                    ImGui::SameLine(); // 横並びにしたい場合
                }

                ImGui::Text(" ");
                ImGui::Text(" ");
            }
            
            // lights
            if(ImGui::CollapsingHeader("Lights", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("Sky Light [Radiance]");
                ImGui::DragFloat3("RGB0", &scene_manager.sky_light_rgba.x, 0.01f);
                ImGui::DragFloat("Power0 [W*Sr-1*M-2]", &scene_manager.sky_light_rgba.a, 0.1f);
                scene_manager.sky_light_rgba.x = std::clamp<float>(scene_manager.sky_light_rgba.x, 0.0, 1.0);
                scene_manager.sky_light_rgba.y = std::clamp<float>(scene_manager.sky_light_rgba.y, 0.0, 1.0);
                scene_manager.sky_light_rgba.z = std::clamp<float>(scene_manager.sky_light_rgba.z, 0.0, 1.0);
                scene_manager.sky_light_rgba.a = std::clamp<float>(scene_manager.sky_light_rgba.a, 0.0, scene_manager.directional_light_rgba.a/(2.0*3.1415));

                ImGui::Text("Directional Light [Vertical Radiance]");
                ImGui::DragFloat3("RGB1", &scene_manager.directional_light_rgba.x, 0.01f);
                ImGui::DragFloat3("Dir", &scene_manager.directional_light_dir.x, 0.01f);
                ImGui::DragFloat("Power1 [W*M-2]", &scene_manager.directional_light_rgba.a, 0.1f);
                scene_manager.directional_light_rgba.x = std::clamp<float>(scene_manager.directional_light_rgba.x, 0.0, 1.0);
                scene_manager.directional_light_rgba.y = std::clamp<float>(scene_manager.directional_light_rgba.y, 0.0, 1.0);
                scene_manager.directional_light_rgba.z = std::clamp<float>(scene_manager.directional_light_rgba.z, 0.0, 1.0);
                scene_manager.directional_light_rgba.a = std::clamp<float>(scene_manager.directional_light_rgba.a, 0.0, 500.0);

                scene_manager.directional_light_dir.x = std::clamp<float>(scene_manager.directional_light_dir.x, -0.7, 0.7);
                scene_manager.directional_light_dir.y = std::clamp<float>(scene_manager.directional_light_dir.y, 0.1, 1.0);
                scene_manager.directional_light_dir.z = std::clamp<float>(scene_manager.directional_light_dir.z, -0.7, 0.7);

                ImGui::Text(" ");
            }

            if(ImGui::CollapsingHeader("ProbeTraceTime", ImGuiTreeNodeFlags_DefaultOpen)){
                probe_trace_time_total += probe_trace_time;
                probe_trace_time_num++;
                ImGui::Text("%lf [ms]", probe_trace_time_total/double(probe_trace_time_num));

                if(ImGui::Button("Time Correct Reset")){
                    probe_trace_time = 0.0;
                    probe_trace_time_total = 0.0;
                    probe_trace_time_num = 0;
                }
                ImGui::Text(" ");
            }
            
            if(ImGui::CollapsingHeader("Screen Shot", ImGuiTreeNodeFlags_DefaultOpen)){
                if(export_screen_shot_tonemap_srgb) export_screen_shot_tonemap_srgb = false;
                if(screen_shot_tonemap_srgb) export_screen_shot_tonemap_srgb = true;
                screen_shot_tonemap_srgb = ImGui::Button("tonemap_srgb");

                if(export_screen_shot_raw_radiance) export_screen_shot_raw_radiance = false;
                if(screen_shot_raw_radiance) export_screen_shot_raw_radiance = true;
                screen_shot_raw_radiance = ImGui::Button("raw_radiance");
                ImGui::Text(" ");
            }

            if(ImGui::CollapsingHeader("LOD AS Size [MB]", ImGuiTreeNodeFlags_DefaultOpen)){
                ImGui::Text("lod0: %.1f, lod1234: %.1f,  total: %.1f", 
                    ((float)lod0_as_size)/1024.f/1204.f, 
                    ((float)lod1234_as_size)/1024.f/1024.f,
                    ((float)lod1234_as_size+lod0_as_size)/1024.f/1024.f
                );
                ImGui::Text(" ");
            }

            ImGui::Text(" ");
            if(ImGui::RadioButton("is transpose", is_transpose))
            {
                is_transpose = !is_transpose;
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
            
            // guizmo
            ImGuizmo::BeginFrame();
            ImGuizmo::SetRect(0, 0, render_image.image_create_info.extent.width, render_image.image_create_info.extent.height);
            ImGuizmo::SetGizmoSizeClipSpace(0.25f);
            ImGuizmo::OPERATION operation = ImGuizmo::TRANSLATE;
            //ImGuizmo::OPERATION operation = ImGuizmo::ROTATE;
            is_using_guizmo = ImGuizmo::IsUsing();

            // ギズモを画面全体に描画
            glm::f32mat4x4* model_mat = nullptr;
            if(selected_model_name && selected_inst_name){
                if(scene_manager.models_instances.contains(selected_model_name.value())){
                    if(scene_manager.models_instances[selected_model_name.value()].contains(selected_inst_name.value())){
                        
                        model_mat = &scene_manager.models_instances[selected_model_name.value()][selected_inst_name.value()].transform;

                    }else{
                        selected_model_name = std::nullopt;
                        selected_inst_name = std::nullopt;
                    }

                }else{
                    selected_model_name = std::nullopt;
                    selected_inst_name = std::nullopt;
                }
            }

            if(model_mat != nullptr){
                glm::f32mat4x4 projection = scene_manager.camera.projection;
                projection[1][1] *= -1;
                ImGuizmo::Manipulate(
                    &scene_manager.camera.view[0][0],
                    &projection[0][0],
                    operation,
                    ImGuizmo::WORLD,
                    &(*model_mat)[0][0]
                );
            }

        });
    }
};


}