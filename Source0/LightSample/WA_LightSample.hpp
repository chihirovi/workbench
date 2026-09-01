#pragma once

#include "../Scene/SceneManager.hpp"
#include "../Core/Common/Buffer.hpp"

namespace Irori::WA_LightSample{

struct WA_Entry{
    uint32_t index;
    float threshold;
};
    
struct LightSample{
    float position[3];
    float normal[3];
    float emittanse[3];
    float ucw; 
};

struct LightPrimitive{
    float ucw; // = 1 / [A_prim / sum(A_prim)];
};

struct LightPrimitiveWaTableRange{
    uint32_t base;
    uint32_t count;
};

struct LightObject{
    LightPrimitiveWaTableRange primitive_wa_table_range;
    uint32_t render_instance_base;
    uint32_t render_instance_index;
    glm::f32mat4x4 scene_model_transform; // instance の変換行列を含まない, = scene::model_inst.transform
    float surface_total_area; // 変換なしの面積
    float ucw; // = 1 / [ A_obj * Intensity / sum(A_obj * Intensity) ];
};

class LightSampleBuiler{
public:
    Core::VulkanContext* vc = nullptr;
    std::vector<WA_Entry> light_object_pdf_wa_table;
    std::vector<LightObject> light_object_table;
    std::unordered_map<std::string, std::vector<LightObject>> model_name_to_light_objects_map; // 後で検索する用
    std::vector<WA_Entry> light_primitive_pdf_wa_table;
    std::unordered_map<uint64_t, std::vector<Render::VertexType0>> vertex_cache; // vertex buffer の device address を使う
    std::unordered_map<uint64_t, std::vector<uint32_t>> index_cache; // index buffer の device address を使う

public:
    LightSampleBuiler(){;}
    LightSampleBuiler(Core::VulkanContext& vc_){vc = &vc_;}
    void build_light_primitive(Scene::UniqueSceneManager& scene_manager);
    void build_light_object(Scene::UniqueSceneManager& scene_manager);
    std::vector<float> calc_primitive_areas(Render::Instance& instance, Core::UniqueBuffer* vb, Core::UniqueBuffer* ib);
    std::vector<WA_Entry> calc_wa_table(std::vector<float>& _weights, float weights_sum);
    std::vector<WA_Entry> calc_wa_table2(const std::vector<float>& _weights, float weights_sum);
    void print_light_primitive_table();
    void print_light_object_table();
    
    void copy_to_buffer(
        Core::UniqueBuffer&     light_object_pdf_wa_table_buffer,
        Core::UniqueBuffer&     light_object_table_buffer,
        Core::UniqueBuffer&     light_primitive_pdf_wa_table_buffer
    );
};

}









