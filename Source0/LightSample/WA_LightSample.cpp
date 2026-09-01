
#include "WA_LightSample.hpp"
#include <ranges>
#include <stack>
#include <numeric>
#include <chrono>

namespace Irori::WA_LightSample{
    
void LightSampleBuiler::build_light_primitive(Scene::UniqueSceneManager& scene_manager){
    for(auto& [model_name, _] : scene_manager.model_manager.models_data){

        // モデルにを探して，それを構成する render instance に分解する．
        auto [render_instances, _] = scene_manager.model_manager.get_render_instances_mesh_prim_ids(model_name, 0, 0);
        auto materials = scene_manager.model_manager.get_render_materials(model_name, 0, 0);
        auto vi_buffers = scene_manager.model_manager.get_render_instances_VIbuffers(model_name);
        
        // 個々の emissive render instance に対して，primitive の表面積を計算
        for(const auto& [i, render_instance] : render_instances|std::views::enumerate){

            // emissive だけを選択する
            auto& material = materials[render_instance.material_id];
            glm::f32vec3 emissive_factor = glm::f32vec3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);

            if((material.texture_view_id_emissive < 0) && (glm::length(emissive_factor) == 0.0f)){
                continue;
            }else{
                //printf("%d, %f, %f, %f\n", material.texture_view_id_emissive, emissive_factor.r, emissive_factor.g, emissive_factor.b);
            }
            
            // primitive の 表面積を計算
            auto primitive_areas = calc_primitive_areas(render_instance, vi_buffers[i].first, vi_buffers[i].second);
            
            // wa table を構築
            float weights_sum = std::reduce(primitive_areas.begin(), primitive_areas.end());
            printf("%f\n", weights_sum);
            auto prim_pdf_table = calc_wa_table2(primitive_areas, weights_sum);
            
            // light object を作る
            LightObject light_object{
                .primitive_wa_table_range{
                    .base = uint32_t(light_primitive_pdf_wa_table.size()),
                    .count = uint32_t(prim_pdf_table.size()),
                },
                //.render_instance_base = // 実行時に決まる
                .render_instance_index = uint32_t(i),
                //.scene_model_transform // 実行時に決まる
                .surface_total_area = weights_sum,
                //.ucw = // 実行時に決まる
            };
            model_name_to_light_objects_map[model_name].push_back(light_object);
            
            // pdf table 保管
            light_primitive_pdf_wa_table.insert(
                light_primitive_pdf_wa_table.end(),
                prim_pdf_table.begin(),
                prim_pdf_table.end()
            );
        }
    }
}

void LightSampleBuiler::build_light_object(Scene::UniqueSceneManager& scene_manager){
    
    light_object_table.resize(0);
    std::vector<float> light_object_weights;
    light_object_weights.reserve(1024*128);

    for(auto& [model_name, scene_model_insts] : scene_manager.models_instances){

        uint32_t render_inst_base = scene_manager.model_name_to_render_inst_table[model_name].first;

        auto start = std::chrono::system_clock::now(); // 計測開始時間
                                                       //
        auto [render_instances, _] = scene_manager.model_manager.get_render_instances_mesh_prim_ids(model_name, 0, 0);

        auto end = std::chrono::system_clock::now();  // 計測終了時間
        double elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count(); //処理に要した時間をミリ秒に変換
        printf("time: %f ms\n", (float)elapsed);

        auto materials = scene_manager.model_manager.get_render_materials(model_name, 0, 0);

                                                       
        for(auto& [_, scene_model_inst] : scene_model_insts){

            for(LightObject light_obj : model_name_to_light_objects_map[model_name]){
                // light object
                light_obj.render_instance_base = render_inst_base;
                light_obj.scene_model_transform = scene_model_inst.transform;
                
                light_object_table.push_back(light_obj);                      

                auto& material = materials[render_instances[light_obj.render_instance_index].material_id];

                // weight
                glm::f32vec3 emissive_factor = glm::f32vec3(material.emissive_factor[0], material.emissive_factor[1], material.emissive_factor[2]);
                float intensity = glm::length(emissive_factor);
                
                /*
                float pi = 3.1415926535;
                float p = 1.6075;
                float a = light_obj.scene_model_transform[0][0];
                float b = light_obj.scene_model_transform[1][1];
                float c = light_obj.scene_model_transform[2][2];
                float a_p = pow(a, p);
                float b_p = pow(b, p);
                float c_p = pow(c, p);
                float weight = 4.0 * pi * pow((a_p * b_p + b_p * c_p + c_p * a_p)/3.0, 1.0/p);
                weight *= light_obj.surface_total_area * intensity;
                */

                float weight = light_obj.surface_total_area * intensity;
                light_object_weights.push_back(weight);
            }

        }

    }
    
    //
            
    float weights_sum = std::reduce(light_object_weights.begin(), light_object_weights.end());
    auto light_object_pdf_table = calc_wa_table2(light_object_weights, weights_sum);

    // light_object_pdf_wa_table
    light_object_pdf_wa_table.resize(0);
    light_object_pdf_wa_table.insert(
        light_object_pdf_wa_table.end(),
        light_object_pdf_table.begin(),
        light_object_pdf_table.end()
    );
    
    // light object table
    for(int i = 0; i < light_object_table.size(); i++){
        light_object_table[i].ucw = weights_sum / light_object_weights[i];        
    }
}

std::vector<float> LightSampleBuiler::calc_primitive_areas(Render::Instance& instance, Core::UniqueBuffer* vb, Core::UniqueBuffer* ib){

    // check vertex cache 
    /*
    if(vertex_cache.contains(vb->get_device_address()) == false){
        std::vector<Render::VertexType0> tmp_verts(instance.raw_vertex_num);
        vb->download_data(tmp_verts.data(), tmp_verts.size() * sizeof(Render::VertexType0));
        vertex_cache[vb->get_device_address()] = tmp_verts;
    }

    std::vector<Render::VertexType0>& tmp_verts = vertex_cache[vb->get_device_address()];
    */
    
    std::vector<Render::VertexType0> tmp_verts(instance.raw_vertex_num);
    vb->download_data(tmp_verts.data(), tmp_verts.size() * sizeof(Render::VertexType0));

    // check index cache
    /*
    if(index_cache.contains(ib->get_device_address()) == false){
        std::vector<uint32_t> indexs(instance.raw_triangle_num * 3);
        ib->download_data(indexs.data(), indexs.size() * sizeof(uint32_t));
        index_cache[ib->get_device_address()] = indexs;
    }

    std::vector<uint32_t>& indexs = index_cache[ib->get_device_address()];
    */

    std::vector<uint32_t> indexs(instance.raw_triangle_num * 3);
    ib->download_data(indexs.data(), indexs.size() * sizeof(uint32_t));

    // calc areas

    // vertex transform
    std::vector<glm::f32vec3> vertexs(instance.raw_vertex_num);

    for(const auto& [i, v] : tmp_verts|std::views::enumerate){

        auto p = glm::f32vec4(v.position[0], v.position[1], v.position[2], 1.0f);
        
        vertexs[i] = (instance.local_model_matrix * p);
    }
    
    // calc primitive areas
    std::vector<float> areas(instance.raw_triangle_num);

    for(int t = 0; t < instance.raw_triangle_num; t++){
        auto i0 = indexs[t*3 + 0];
        auto i1 = indexs[t*3 + 1];
        auto i2 = indexs[t*3 + 2];

        auto v01 = vertexs[i1] - vertexs[i0];
        auto v02 = vertexs[i2] - vertexs[i0];
        
        areas[t] = 0.5f * glm::length(glm::cross(v01, v02));
    }

    // recode cache
    return areas;
}

std::vector<WA_Entry> LightSampleBuiler::calc_wa_table(std::vector<float>& _weights, float weights_sum){
    std::vector<float> weights = _weights; // copy
    
    float average = weights_sum / weights.size();
    
    std::stack<uint32_t> large, small;
    
    for(const auto& [i, e] : weights|std::views::enumerate){
        if(e <= average) small.push(i);
        else large.push(i);
    }
    
    std::vector<WA_Entry> pdf_table(weights.size());
    for(int i=0; i<pdf_table.size(); i++){
        pdf_table[i].index = i;
    }
    
    while((!small.empty()) && (!large.empty())){

        uint32_t j = small.top();
        small.pop();
        
        uint32_t k = large.top();
        
        pdf_table[j].index = k;
        weights[k] = weights[k] - (average - weights[j]);
        
        if(weights[k] <= average){
            small.push(k);
            large.pop();
        }
    }
    
    for(const auto& [i, e] : pdf_table|std::views::enumerate){
        e.threshold = weights[i]/average;
    }

    return pdf_table;
}

std::vector<WA_Entry> LightSampleBuiler::calc_wa_table2(const std::vector<float>& _weights, float weights_sum){
    // 1. ワーク用の重み配列（コピー）
    std::vector<float> weights = _weights; 
    size_t n = weights.size();
    float average = weights_sum / n;
    
    // stackの代わりにvectorを使うことで高速化
    std::vector<uint32_t> small, large;
    small.reserve(n);
    large.reserve(n);
    
    for (size_t i = 0; i < n; ++i) {
        if (weights[i] <= average) small.push_back(i);
        else large.push_back(i);
    }
    
    std::vector<WA_Entry> pdf_table(n);
    
    // 2. エイリアス（相方）の割り当てループ
    while (!small.empty() && !large.empty()) {
        uint32_t j = small.back();
        small.pop_back();
        
        uint32_t k = large.back();
        // largeは pop しない（まだ余りがある可能性があるため）
        
        pdf_table[j].index = k;
        pdf_table[j].threshold = weights[j] / average; // 正しい位置で threshold を確定させる！
        
        // large側の重みを削る
        weights[k] = weights[k] - (average - weights[j]);
        
        if (weights[k] <= average) {
            small.push_back(k);
            large.pop_back(); // ここで初めて large から取り除く
        }
    }
    
    // 3. 最後に残った要素の処理（これらはそのスロットを100%占有する）
    while (!large.empty()) {
        uint32_t g = large.back();
        large.pop_back();
        pdf_table[g].index = g;
        pdf_table[g].threshold = 1.0f;
    }
    while (!small.empty()) {
        uint32_t l = small.back();
        small.pop_back();
        pdf_table[l].index = l;
        pdf_table[l].threshold = 1.0f;
    }

    return pdf_table;   
}

void LightSampleBuiler::print_light_primitive_table(){
    for(auto& [model_name, light_objects] : model_name_to_light_objects_map){
        printf("%s\n", model_name.c_str());

        for(auto& light_object : light_objects){
            auto base = light_object.primitive_wa_table_range.base;
            auto count = light_object.primitive_wa_table_range.count;
            
            for(int i = 0; i < count; i++){
                printf("    ");
                printf("%d: %d, %f\n", i, light_primitive_pdf_wa_table[base + i].index, light_primitive_pdf_wa_table[base+i].threshold);
            }
        }

        printf("\n");
    }
}

void LightSampleBuiler::print_light_object_table(){
    printf("size : %d\n", light_object_table.size());
    for(auto& obj : light_object_table){
        printf("    range:          %d, %d\n", obj.primitive_wa_table_range.base, obj.primitive_wa_table_range.count);
        printf("    base, index:    %d, %d\n", obj.render_instance_base, obj.render_instance_index);
        printf("    area:           %f\n", obj.surface_total_area);
        printf("    ucw             %f\n", obj.ucw);
        printf("\n");
    }
    /*
    printf("size : %d\n", light_object_pdf_wa_table.size());
    for(auto& e : light_object_pdf_wa_table){
        printf("index: %d, threshold: %f\n", e.index, e.threshold);
    }
    printf("\n");
    */
}

void LightSampleBuiler::copy_to_buffer(
    Core::UniqueBuffer&     light_object_pdf_wa_table_buffer,
    Core::UniqueBuffer&     light_object_table_buffer,
    Core::UniqueBuffer&     light_primitive_pdf_wa_table_buffer
){
    light_object_pdf_wa_table_buffer.transfer_data(
        light_object_pdf_wa_table.data(),
        light_object_pdf_wa_table.size() * sizeof(WA_Entry)
    );

    light_object_table_buffer.transfer_data(
        light_object_table.data(),
        light_object_table.size() * sizeof(LightObject)
    );

    light_primitive_pdf_wa_table_buffer.transfer_data(
        light_primitive_pdf_wa_table.data(),
        light_primitive_pdf_wa_table.size() * sizeof(WA_Entry)
    );
}

}






