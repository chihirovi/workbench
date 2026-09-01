#pragma once
#include "../Render/VertexType.hpp"
#include "../Render/Cluster.hpp"
#include "../Render/Instance.hpp"
#include "../Render/Material.hpp"
#include "../Render/TextureView.hpp"
#include "../Core/Common/VulkanContext.hpp"
#include "../Core/Common/Image.hpp"
#include "../Core/Common/Buffer.hpp"
#include "../Core/Common/Sampler.hpp"
#include "DdsLoader.hpp"

#include <tinygltf/tiny_gltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
            
#include <functional>
#include <ranges>

#define MAX_MATERIAL_NUM        1024
#define MAX_TEXTURE_NUM         1024
#define MAX_TEXTURE_VIEW_NUM    1024*8

namespace Irori::Scene{

// ----------------------------------------------------
// RenderInstanceDesc, Render::Instanceを作るための情報
struct RenderInstanceDesc{
    uint32_t cluster_buffer_idx;    
    uint32_t cluster_vertex_buffer_idx;
    uint32_t cluster_index_buffer_idx;
    uint32_t cluster_bvh8_buffer_idx;
    uint32_t cluster_num;
    uint32_t raw_vertex_buffer_idx;
    uint32_t raw_index_buffer_idx;
    uint32_t raw_vertex_num;
    uint32_t raw_index_num;
    uint32_t raw_triangle_num;
    int32_t  material_desc_idx;     //==-1でdefault material
    std::string name;
    uint32_t mesh_id;
    uint32_t primitive_id;
};



// ----------------------------------------------------
// RenderMaterialDesc, Render::Materialを作るための情報
struct RenderMaterialDesc{
    Render::Material data;
    std::string name;
};



// ----------------------------------------------------
// NodeGraph, gltfのnodeをそのまま形にしたもの，
// 将来的に動きを扱うかもだから，tree構造はそのまま
struct NodeGraph{
    std::vector<uint32_t>   render_instance_desc_indices;
    glm::f32mat4x4          model_matrix;
    std::vector<NodeGraph>  children;
    std::string             name;
};



// ----------------------------------------------------
// Model data, modelの情報が丸ごと入ってる，
// 1frame分のrenderingに必要な情報はここから, その都度組み立てる
struct UniqueModelData{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueModelData);

public:
    // texture
    std::vector<std::optional<Core::UniqueImage>>      texture_table;
    std::vector<Render::TextureView>    texture_view_table;

    // material data
    std::vector<RenderMaterialDesc>     material_descs;
    
    // virtual geometry data
    bool                                has_virtual_geometry;
    std::vector<Core::UniqueBuffer>     cluster_buffers;
    std::vector<Core::UniqueBuffer>     cluster_vertex_buffers;
    std::vector<Core::UniqueBuffer>     cluster_index_buffers;
    std::vector<Core::UniqueBuffer>     cluster_bvh_buffers;
    
    // raw vertex index
    std::vector<Core::UniqueBuffer>     raw_vertex_buffers;
    std::vector<Core::UniqueBuffer>     raw_index_buffers;

    // render instance data
    std::vector<RenderInstanceDesc>     instance_descs;

    // node graph
    NodeGraph                           node_graph_root;
    
public:
    UniqueModelData() = default;
    UniqueModelData(BOOST_RV_REF(UniqueModelData) rhs) = default;               //move constractor
    UniqueModelData& operator=(BOOST_RV_REF(UniqueModelData) rhs) = default;    //move assignment
};



// ----------------------------------------------------
// UniqueModelManager, すべての統括，管理をする
class UniqueModelManager{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueModelManager);

public:
    Core::VulkanContext*                                vulkan_context = nullptr;
    std::unordered_map<std::string, UniqueModelData>    models_data;
    Core::UniqueImage                                   dummy_texture;

public:
    UniqueModelManager(){;}
    UniqueModelManager(Core::VulkanContext* vc){
        vulkan_context = vc;

        // dummy texture
        uint8_t green_data[4] = {0, 255, 0, 0};
        dummy_texture = std::move(Core::UniqueImage::Builder2D(vulkan_context)
        .set_size_format_usage_layout(1,1,vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal)
        .set_data(green_data)
        .build());
    };

    UniqueModelManager(BOOST_RV_REF(UniqueModelManager) rhs) = default;               //move constractor
    UniqueModelManager& operator=(BOOST_RV_REF(UniqueModelManager) rhs) = default;    //move assignment
                                                                                      
    void load_textures(tinygltf::Model& model, UniqueModelData& model_data, std::unordered_map<int, DDS_Vk_Data>& dds_images_data);
    void load_materials(tinygltf::Model& model, UniqueModelData& model_data);
    void load_meshs_nodes(tinygltf::Model& model, UniqueModelData& model_data, std::string& mode_name);
    UniqueModelManager& register_model(std::string model_name, std::string gltf_file_path, std::optional<std::string> ivg_file_name);
    bool has_model_name(std::string model_name){return models_data.contains(model_name);}
    bool has_virtual_geometry(std::string model_name);

    std::vector<vk::ImageView> get_texture_vkimageviews(std::string model_name);
    std::vector<Render::TextureView> get_texture_views(std::string model_name);
    std::vector<Render::Material> get_render_materials(std::string model_name, uint32_t texture_id_base, uint32_t texture_view_id_base);
    std::pair<std::vector<Render::Instance>, std::vector<std::pair<uint32_t, uint32_t>>> get_render_instances_mesh_prim_ids(std::string model_name, uint32_t material_id_base, uint32_t default_material_id);
    std::vector<std::pair<Core::UniqueBuffer*, Core::UniqueBuffer*>> get_render_instances_VIbuffers(std::string model_name);
    
    static Render::Material get_default_material();
    
    void print_mesh_data(std::string model_name);
};


};