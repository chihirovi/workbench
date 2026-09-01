#include "ModelManager.hpp"
#include "LoadVertexIndex.hpp"
#include "../VirtualGeometry/VG_File.hpp"
#include "DdsLoader.hpp"

namespace Irori::Scene{

Render::Material UniqueModelManager::get_default_material(){

    // make default material
    Render::Material default_material{
        .is_double_side             = 0,
        .blend_mode                 = Render::MaterialBlendMode::opaque,
        .alpha_cut_off              = 0.5,

        .base_color_factor          = {1.0, 1.0, 1.0, 1.0},
        .emissive_factor            = {0.0, 0.0, 0.0, 1.0},
        .metallic_factor            = 1.0,
        .roughness_factor           = 1.0,
        .normal_scale               = 1.0,
        .occlusion_strength         = 1.0,
        
        .texture_id_base            = 0,
        .texture_view_id_base       = 0,

        .texture_view_id_albedo     = -1,
        .texture_view_id_normal     = -1,
        .texture_view_id_metallic_roughness = -1,
        .texture_view_id_occlusion  = -1,
        .texture_view_id_emissive   = -1,
        
        .is_pbrSpecularGlossiness_material = 0, // false
        .specular_factor            = {1.0, 1.0, 1.0},
        .is_transmission            = 0,
    };
    
    return default_material;
}


void UniqueModelManager::load_textures(
    tinygltf::Model& model, 
    UniqueModelData& model_data,
    std::unordered_map<int, DDS_Vk_Data>& dds_images_data){
    
    bool is_used_MSFT_texture_dds = false;
    for(auto& ext : model.extensionsUsed){
        if(ext == "MSFT_texture_dds"){
            is_used_MSFT_texture_dds = true;
            printf("hello\n");
            break;
        }
    }

    // 
    for(const auto& [image_index, image] : model.images | std::views::enumerate){

        // dds image
        if(dds_images_data.contains(image_index)){
            auto& dds_vk_data = dds_images_data[image_index];
            Core::UniqueImage tex_image = Core::UniqueImage::Builder2D_DDS(vulkan_context)       
                .set_usage_layout(vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal)
                .set_dds_vk_data(dds_vk_data)
                .build();
            
            model_data.texture_table.push_back(std::move(tex_image));           

            continue;
        }
        
        // dds でない png があるなら飛ばす
        if(is_used_MSFT_texture_dds){
            if((image.component < 0) && (image.bits < 0)){
                model_data.texture_table.push_back(std::nullopt);
                continue;
            }
        }

        // common image
        ASSERT_WITH_MSG((image.component == 4 && image.bits == 8), std::format("Irori only support 4 component 8bit texture, here is [{}] component [{}] bit, {}", image.component, image.bits, image.uri));
        Core::UniqueImage tex_image = Core::UniqueImage::Builder2D(vulkan_context)       
            .set_size_format_usage_layout(image.width, image.height, vk::Format::eR8G8B8A8Unorm, vk::ImageUsageFlagBits::eSampled, vk::ImageLayout::eShaderReadOnlyOptimal)
            .set_data(image.image.data())
            .enable_mipmap()
            .build();
        model_data.texture_table.push_back(std::move(tex_image));
    }
    
    for(auto& texture : model.textures){

        // sampler を決める
        // デフォルトはtrilinearが一般的らしい
        Core::UniqueSamplerPool::TYPE sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_LINEAR_MIP_LINEAR;

        if(texture.sampler >= 0){

            auto& sampler = model.samplers[texture.sampler];
            
            //0
            if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_NEAREST_MIP_NONE;

            //1
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_LINEAR_MIP_NONE;
            //2
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_NEAREST_MIP_NEAREST;
            //3
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_LINEAR_MIP_NEAREST;
            //4
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_NEAREST_MIP_LINEAR;
            //5
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_NEAREST && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_NEAREST_MIN_LINEAR_MIP_LINEAR;
            //6
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_NEAREST_MIP_NONE;
            //7
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_LINEAR_MIP_NONE;
            //8
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_NEAREST_MIP_NEAREST;
            //9
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_NEAREST){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_LINEAR_MIP_NEAREST;
            //10
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_NEAREST_MIPMAP_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_NEAREST_MIP_LINEAR;
            //11
            }else if(sampler.magFilter == TINYGLTF_TEXTURE_FILTER_LINEAR && sampler.minFilter == TINYGLTF_TEXTURE_FILTER_LINEAR_MIPMAP_LINEAR){
                sampler_type = Core::UniqueSamplerPool::TYPE::MAG_LINEAR_MIN_LINEAR_MIP_LINEAR;

            }else{
                printf("warning : unknown sampler type, mag : %d, min : %d\n", sampler.magFilter, sampler.minFilter);
            }

        }

        // texture view を作る
        Render::TextureView texture_view{
            .texture_id = (texture.source >= 0) ? uint32_t(texture.source) : 0,
            .sampler_type = sampler_type,
        };

        if(texture.extensions.contains("MSFT_texture_dds")){
            texture_view.texture_id = texture.extensions["MSFT_texture_dds"].Get("source").GetNumberAsInt();           
            //printf("%d\n", texture_view.texture_id);
        }

        model_data.texture_view_table.push_back(texture_view);
    }
}

void UniqueModelManager::load_materials(tinygltf::Model& model, UniqueModelData& model_data){

    for(auto& material : model.materials){

        Render::Material render_material = get_default_material();
        
        // double side
        render_material.is_double_side = (material.doubleSided?1:0);   
        
        // blend
        if(material.alphaMode == "OPAQUE") render_material.blend_mode = Render::MaterialBlendMode::opaque;
        if(material.alphaMode == "BLEND") render_material.blend_mode = Render::MaterialBlendMode::blend;
        if(material.alphaMode == "MASK") render_material.blend_mode = Render::MaterialBlendMode::mask;   
        render_material.alpha_cut_off = material.alphaCutoff;
        
        // texture id
        auto get_texture_view_id = [&]<typename T>(T& tex_info){
            if(tex_info.index >= 0){
                ASSERT_WITH_MSG((tex_info.texCoord == 0), "Irori only support texcoord 0 ");
                return tex_info.index;
            }else{
                return -1;
            }
        };
        
        // albedo, default value = vec4(1.0, 1.0, 1.0, 1.0)
        for(int i=0; i<4; i++){
            render_material.base_color_factor[i] = material.pbrMetallicRoughness.baseColorFactor[i];
        }
        render_material.texture_view_id_albedo = get_texture_view_id(material.pbrMetallicRoughness.baseColorTexture);

        // metalic roughness, default value = vec2(1.0, 1.0)
        render_material.metallic_factor = material.pbrMetallicRoughness.metallicFactor;
        render_material.roughness_factor = material.pbrMetallicRoughness.roughnessFactor;
        render_material.texture_view_id_metallic_roughness = get_texture_view_id(material.pbrMetallicRoughness.metallicRoughnessTexture);

        
        // emmisive, default value = vec4(0.0, 0.0, 0.0 ,1.0)
        for(int i=0; i<3; i++){
            render_material.emissive_factor[i] = material.emissiveFactor[i]; //defaule = vec3(0.0)
        }
        render_material.emissive_factor[3] = 1.0;
        if(material.extensions.contains("KHR_materials_emissive_strength")){
            if(material.extensions["KHR_materials_emissive_strength"].Has("emissiveStrength")){
                render_material.emissive_factor[3] = (float)material.extensions["KHR_materials_emissive_strength"].Get("emissiveStrength").GetNumberAsDouble();
            }
        }
        render_material.texture_view_id_emissive = get_texture_view_id(material.emissiveTexture);

        if(material.emissiveTexture.index >= 0 &&( // emissive texture があるのに，factor が 0 なのは都合が悪い
            material.emissiveFactor[0] == 0.0f &&
            material.emissiveFactor[1] == 0.0f &&
            material.emissiveFactor[2] == 0.0f)){

            render_material.emissive_factor[0] = render_material.emissive_factor[1] = render_material.emissive_factor[2] = 1.0f;
        }
        
        // normal, default value = vec4(0.0, 0.0, 0.5, 0.0)
        render_material.normal_scale = material.normalTexture.scale;
        render_material.texture_view_id_normal = get_texture_view_id(material.normalTexture);

        // occlusion, default value = vec4(0.0, 0.0, 0.5, 0.0)
        render_material.occlusion_strength = material.occlusionTexture.strength;
        render_material.texture_view_id_occlusion = get_texture_view_id(material.occlusionTexture);
        
        // KHR_materials_pbrSpecularGlossiness
        if(material.extensions.contains("KHR_materials_pbrSpecularGlossiness")){

            render_material.is_pbrSpecularGlossiness_material = 1; //true

            auto& spec_gloss_ext = material.extensions["KHR_materials_pbrSpecularGlossiness"];

            // diffuse factor
            if(spec_gloss_ext.Has("diffuseFactor")){
                auto& diffuse_factor = spec_gloss_ext.Get("diffuseFactor");
                render_material.base_color_factor[0] = diffuse_factor.Get(0).GetNumberAsDouble();
                render_material.base_color_factor[1] = diffuse_factor.Get(1).GetNumberAsDouble();
                render_material.base_color_factor[2] = diffuse_factor.Get(2).GetNumberAsDouble();
                render_material.base_color_factor[3] = diffuse_factor.Get(3).GetNumberAsDouble();
            }
            
            // diffuse texture
            if(spec_gloss_ext.Has("diffuseTexture")){
                int tex_id = spec_gloss_ext.Get("diffuseTexture").Get("index").GetNumberAsInt();
                render_material.texture_view_id_albedo = tex_id;
            }
            
            // specular factor
            if(spec_gloss_ext.Has("specularFactor")){
                auto& specular_factor = spec_gloss_ext.Get("specularFactor");
                render_material.specular_factor[0] = specular_factor.Get(0).GetNumberAsDouble();
                render_material.specular_factor[1] = specular_factor.Get(1).GetNumberAsDouble();
                render_material.specular_factor[2] = specular_factor.Get(2).GetNumberAsDouble();
            }
            
            // glossiness factor
            float glossiness_factor = 1.0f;
            if(spec_gloss_ext.Has("glossinessFactor")){
                glossiness_factor = spec_gloss_ext.Get("glossinessFactor").GetNumberAsDouble();
            }
            render_material.roughness_factor = glossiness_factor; //specular glossiness が有効の場合，roughness_factor は glossiness として使う
            
            // specularGlossinessTexture
            if(spec_gloss_ext.Has("specularGlossinessTexture")){
                float tex_id = spec_gloss_ext.Get("specularGlossinessTexture").Get("index").GetNumberAsInt();
                render_material.texture_view_id_metallic_roughness = tex_id;
            }
        }
        
        // transmission
        if(material.extensions.contains("KHR_materials_transmission")){
            render_material.is_transmission = 1;
        }
        
        // add material
        model_data.material_descs.push_back(RenderMaterialDesc{
            .data = render_material,
            .name = material.name,
        });
    }
}
    
// todo 
// Irori::ClusterFileData, みたいなの作る
void UniqueModelManager::load_meshs_nodes(
    tinygltf::Model& model, 
    UniqueModelData& model_data,
    std::string& model_name){

    // load instances (gltfのmesh.primitiveに相当するもの)
    // gltfのmeshs->primitivesの構造そのまま
    std::vector<std::vector<uint32_t>> mesh_primitive_table;
    mesh_primitive_table.resize(model.meshes.size());
    
    for(auto [mesh_index, mesh] : model.meshes|std::views::enumerate){

        for(auto [primitive_index, primitive] : mesh.primitives|std::views::enumerate){

            // load vertex, index
            std::vector<Render::VertexType0> vertices;
            std::vector<uint32_t> indices;            
            load_vertex_index(model, primitive, vertices, indices);
        
            // make GPU vertex buffer
            auto vertex_buffer = Core::UniqueBuffer::common(
                vulkan_context, 
                sizeof(Render::VertexType0)*vertices.size(), 
                false, 
                vk::BufferUsageFlagBits::eStorageBuffer|
                vk::BufferUsageFlagBits::eVertexBuffer|
                vk::BufferUsageFlagBits::eShaderDeviceAddress|
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
                vk::BufferUsageFlagBits::eTransferDst|
                vk::BufferUsageFlagBits::eTransferSrc, 
                8
            );
            vertex_buffer.transfer_data(vertices.data(), sizeof(Render::VertexType0)*vertices.size());

            // make GPU index buffer
            auto index_buffer = Core::UniqueBuffer::common(
                vulkan_context, 
                sizeof(uint32_t)*indices.size(), 
                false, 
                vk::BufferUsageFlagBits::eStorageBuffer|
                vk::BufferUsageFlagBits::eIndexBuffer|
                vk::BufferUsageFlagBits::eShaderDeviceAddress|
                vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
                vk::BufferUsageFlagBits::eTransferDst|
                vk::BufferUsageFlagBits::eTransferSrc, 
                8
            );
            index_buffer.transfer_data(indices.data(), sizeof(uint32_t)*indices.size());
            
            // bufferの保管先を変える
            model_data.raw_vertex_buffers.push_back(std::move(vertex_buffer));
            model_data.raw_index_buffers.push_back(std::move(index_buffer));
            
            // make indtance data
            RenderInstanceDesc tmp_instance_desc{
                .cluster_buffer_idx         = 0,
                .cluster_vertex_buffer_idx  = 0,
                .cluster_index_buffer_idx   = 0,
                .cluster_num                = 0,
                .raw_vertex_buffer_idx      = uint32_t(model_data.raw_vertex_buffers.size()-1),
                .raw_index_buffer_idx       = uint32_t(model_data.raw_index_buffers.size()-1),
                .raw_vertex_num             = uint32_t(vertices.size()),
                .raw_index_num              = uint32_t(indices.size()),
                .raw_triangle_num           = uint32_t(indices.size()/3),
                .material_desc_idx          = primitive.material >= 0 ? primitive.material : -1,
                .name                       = std::format("{}_{}",mesh.name, primitive_index),
                .mesh_id                    = uint32_t(mesh_index),
                .primitive_id               = uint32_t(primitive_index),
            };
            model_data.instance_descs.push_back(std::move(tmp_instance_desc));
            mesh_primitive_table[mesh_index].push_back(model_data.instance_descs.size()-1);
        }
    }

    // utility
    auto get_model_matrix = [&](tinygltf::Node& node){
        glm::mat4x4 rm(1), tm(1), sm(1);
        if(!node.rotation.empty()){
            glm::quat q(node.rotation[3], node.rotation[0], node.rotation[1], node.rotation[2]);
            rm = glm::mat4_cast(q);
        }
        
        if(!node.translation.empty()){
            glm::vec3 tv = glm::make_vec3(node.translation.data());
            tm = glm::translate(glm::mat4(1), tv);
        }
        
        if(!node.scale.empty()){
            glm::vec3 sv = glm::make_vec3(node.scale.data());
            sm = glm::scale(glm::mat4(1), sv);
        }
        
        glm::f32mat4x4 model_matrix = tm*rm*sm;
        
        if(!node.matrix.empty()){
            model_matrix = glm::make_mat4x4<double>(node.matrix.data());
        }
        
        return model_matrix;
    };

    std::function<void(NodeGraph*, uint32_t)> make_node_graph = [&](NodeGraph* node_graph, uint32_t node_index){

        auto& node = model.nodes[node_index];
        
        // insret instance data index
        if(node.mesh >= 0){
            for(auto instance_data_index : mesh_primitive_table[node.mesh]){
                node_graph->render_instance_desc_indices.push_back(instance_data_index);
            };
        }

        // get model matrix
        node_graph->model_matrix = get_model_matrix(node);
        
        // name
        node_graph->name = node.name;
        
        // children
        node_graph->children.resize(node.children.size());
        for(auto [c_node_index, c_node] : node.children | std::views::enumerate){
            make_node_graph(&(node_graph->children[c_node_index]), c_node);
        }
    };

    // make node graph
    auto& node_root = model_data.node_graph_root;
    node_root.model_matrix = glm::f32mat4x4(1.0f);
    node_root.name = std::format("{}_{}", model_name, "root");
    node_root.children.resize(model.scenes[model.defaultScene].nodes.size());

    for(auto [child_idx, node_idx] : model.scenes[model.defaultScene].nodes | std::views::enumerate){
        make_node_graph(&node_root.children[child_idx], uint32_t(node_idx));
    }
}


// ivg は irori vurtual geometory の略，独自ファイル
UniqueModelManager& UniqueModelManager::register_model(
    std::string model_name, 
    std::string gltf_file_path, 
    std::optional<std::string> ivg_file_path){

    // check
    ASSERT_TRUE_WITH_MSG(has_model_name(model_name), std::format("scene graph alredy has this model name : {}, use other one", model_name));
    
    // register name
    models_data[model_name];
    
    if(ivg_file_path){
        models_data[model_name].has_virtual_geometry = true;
        // load virtual geometry data
        /*
        std::vector<Irori::Render::Cluster> clusters;
        std::vector<uint32_t> verts;
        std::vector<uint8_t> idxs;
        Read_VG_File(ivg_file_path.value(), clusters, verts, idxs);
        */
    }else{
        models_data[model_name].has_virtual_geometry = false;
    }
    
    // custom image loader  
    std::unordered_map<int, DDS_Vk_Data> dds_images_data;
    std::vector<std::vector<uint8_t>> dds_data;

    tinygltf::LoadImageDataFunction LoadImageDataFn = [&](
        tinygltf::Image* image,
        const int image_idx,
        std::string* err,
        std::string* warn,
        int req_width,
        int req_height,
        const unsigned char* bytes,
        int size,
        void* user_data) -> bool {

        printf("hello texture %d\n", image_idx);
        // DDSなら自前
        if (check_dds_header((uint8_t*)bytes))
        {
            dds_data.push_back(std::vector<uint8_t>());
            dds_data.back().resize(size);
            std::memcpy(dds_data.back().data(), bytes, size);
            dds_images_data[image_idx] = load_dds(dds_data.back().data(), size);
            return true;
        }
        
        // PNG/JPGはstb_image
        return tinygltf::LoadImageData(
            image,
            image_idx,
            err,
            warn,
            req_width,
            req_height,
            bytes,
            size,
            user_data
        );
    };
    
    // load gltf model
    tinygltf::Model model;
    tinygltf::TinyGLTF loader;
    std::string err, warn;
    loader.SetImageLoader(LoadImageDataFn, nullptr);
    ASSERT_WITH_MSG((loader.LoadASCIIFromFile(
        &model, 
        &err, 
        &warn, 
        gltf_file_path
    )), std::format("load gltf file error {}", err));
    //if(warn.size() != 0) printf("load gltf file warn : %s\n", warn.c_str());
    
    // load textures
    load_textures(model, models_data[model_name], dds_images_data);
    
    // load material
    load_materials(model, models_data[model_name]);
    
    // load scene graph
    load_meshs_nodes(model, models_data[model_name], model_name);
    
    //
    for(auto& mat : models_data[model_name].material_descs)
    {
        //printf("mat albedo view id: %d \n",mat.data.texture_view_id_albedo);
    }
    printf("\n");
    return *this;
    
}

// vurtual geometry 用のクラスターデータを持ってるかどうか
bool UniqueModelManager::has_virtual_geometry(std::string model_name){
    // check
    ASSERT_WITH_MSG(has_model_name(model_name), std::format("scene graph dosen't contains model name : {}", model_name));
    
    return models_data[model_name].has_virtual_geometry; 
}

std::vector<vk::ImageView> UniqueModelManager::get_texture_vkimageviews(std::string model_name){
    
    //check
    ASSERT_WITH_MSG(has_model_name(model_name), std::format("scene graph dosen't contains model name : {}", model_name));
    
    //----------
    std::vector<vk::ImageView> texture_vkimageviews;
    for(auto& opt_texture : models_data[model_name].texture_table){
        texture_vkimageviews.push_back(
            opt_texture ? 
            opt_texture.value().image_view.get() :
            dummy_texture.image_view.get()
        );
    }
    
    return texture_vkimageviews;
}

std::vector<Render::TextureView> UniqueModelManager::get_texture_views(std::string model_name){
    
    //check
    ASSERT_WITH_MSG(has_model_name(model_name), std::format("scene graph dosen't contains model name : {}", model_name));
    
    //----------
    auto& texture_view_table = models_data[model_name].texture_view_table;
    std::vector<Render::TextureView> texture_views;
    texture_views.insert(texture_views.end(), texture_view_table.begin(), texture_view_table.end());
    
    return texture_views;
}

std::vector<Render::Material> UniqueModelManager::get_render_materials(
    std::string model_name, 
    uint32_t texture_id_base,
    uint32_t texture_view_id_base){

    //check
    ASSERT_WITH_MSG(has_model_name(model_name), std::format("scene graph dosen't contains model name : {}", model_name));
    
    //----------
    std::vector<Render::Material> render_materials;
    for(auto& material_desc : models_data[model_name].material_descs){
        render_materials.push_back(material_desc.data);
        render_materials.back().texture_id_base = texture_id_base;
        render_materials.back().texture_view_id_base = texture_view_id_base;
    }
    
    return render_materials;
}

// この処理が時間食いすぎてヤバイ
std::pair<std::vector<Render::Instance>, std::vector<std::pair<uint32_t, uint32_t>>> UniqueModelManager::get_render_instances_mesh_prim_ids(
    std::string model_name, 
    uint32_t material_id_base,
    uint32_t default_material_id){

    // check
    ASSERT_WITH_MSG(has_model_name(model_name), std::format("scene graph dosen't contains model name : {}", model_name));

    // variables
    std::vector<Render::Instance> render_instances;
    std::vector<std::pair<uint32_t, uint32_t>> mesh_prim_ids;
    auto& model_data = models_data[model_name];
    bool has_vg = has_virtual_geometry(model_name);

    // utility
    std::function<void(NodeGraph&, glm::f32mat4x4&)> traverse_node_graph 
        = [&](NodeGraph& node, glm::f32mat4x4& parent_matrix){
            
        glm::f32mat4x4 transform = parent_matrix * node.model_matrix;
        //glm::f32mat4x4 transform = glm::f32mat4x4(1.0);
        
        for(auto& instance_desc_index : node.render_instance_desc_indices){
            
            auto& instance_desc = model_data.instance_descs[instance_desc_index];
            
            Render::Instance render_instance;

            if(has_vg){
                Render::Instance _render_instance{
                    .cluster_addr           = model_data.cluster_buffers[instance_desc.cluster_buffer_idx].get_device_address(),
                    .cluster_vertex_addr    = model_data.cluster_vertex_buffers[instance_desc.cluster_vertex_buffer_idx].get_device_address(),
                    .cluster_index_addr     = model_data.cluster_index_buffers[instance_desc.cluster_index_buffer_idx].get_device_address(),
                    .raw_vertex_addr        = model_data.raw_vertex_buffers[instance_desc.raw_vertex_buffer_idx].get_device_address(),
                    .raw_index_addr         = model_data.raw_index_buffers[instance_desc.raw_index_buffer_idx].get_device_address(),
                    .cluster_num            = instance_desc.cluster_num,
                    .raw_vertex_num         = instance_desc.raw_vertex_num,
                    .raw_triangle_num       = instance_desc.raw_triangle_num,
                    .material_id_base       = material_id_base,
                    .material_id            = (instance_desc.material_desc_idx >= 0) ? instance_desc.material_desc_idx : (int32_t(default_material_id) - int32_t(material_id_base)),
                    .local_model_matrix     = transform,
                    .local_reverse_transpose_3x3_model_matrix = glm::transpose(glm::inverse(transform)),
                    .bvh8_node              = instance_desc.cluster_bvh8_buffer_idx != UINT32_MAX ? model_data.cluster_bvh_buffers[instance_desc.cluster_bvh8_buffer_idx].get_device_address() : 0,
                };
                render_instance = _render_instance;

            }else{
                Render::Instance _render_instance{
                    .cluster_addr           = 0,
                    .cluster_vertex_addr    = 0,
                    .cluster_index_addr     = 0,
                    .raw_vertex_addr        = model_data.raw_vertex_buffers[instance_desc.raw_vertex_buffer_idx].get_device_address(),
                    .raw_index_addr         = model_data.raw_index_buffers[instance_desc.raw_index_buffer_idx].get_device_address(),
                    .cluster_num            = 0,
                    .raw_vertex_num         = instance_desc.raw_vertex_num,
                    .raw_triangle_num       = instance_desc.raw_triangle_num,
                    .material_id_base       = material_id_base,
                    .material_id            = (instance_desc.material_desc_idx >= 0) ? instance_desc.material_desc_idx : (int32_t(default_material_id) - int32_t(material_id_base)),
                    .local_model_matrix     = transform,
                    .local_reverse_transpose_3x3_model_matrix = glm::transpose(glm::inverse(transform))
                };               

                render_instance = _render_instance;
            }

            render_instances.push_back(render_instance);
            mesh_prim_ids.push_back({instance_desc.mesh_id, instance_desc.primitive_id});
        }
        
        // traverse chileren
        for(auto& child_node : node.children){
            traverse_node_graph(child_node, transform);
        }
    };
    
    // traverse node graph, make vec<render instance>
    auto& node_graph_root = models_data[model_name].node_graph_root;
    traverse_node_graph(node_graph_root, node_graph_root.model_matrix);

    // end
    return {render_instances, mesh_prim_ids};
}

std::vector<std::pair<Core::UniqueBuffer*, Core::UniqueBuffer*>> UniqueModelManager::get_render_instances_VIbuffers(std::string model_name){
 
    auto& model_data = models_data[model_name];

    std::vector<std::pair<Core::UniqueBuffer*, Core::UniqueBuffer*>> VIbuffers;

    // utility
    std::function<void(NodeGraph&)> traverse_node_graph 
        = [&](NodeGraph& node){
        
        for(auto& instance_desc_index : node.render_instance_desc_indices){
            
            auto& instance_desc = model_data.instance_descs[instance_desc_index];
            
            VIbuffers.push_back({
                &model_data.raw_vertex_buffers[instance_desc.raw_vertex_buffer_idx],
                &model_data.raw_index_buffers[instance_desc.raw_index_buffer_idx],
            });
        }   

        // traverse chileren
        for(auto& child_node : node.children){
            traverse_node_graph(child_node);
        }
    };


    // traverse
    auto& node_graph_root = models_data[model_name].node_graph_root;
    traverse_node_graph(node_graph_root);

    return VIbuffers;
}


void UniqueModelManager::print_mesh_data(std::string model_name){
    auto& model_data = models_data[model_name];
    
    // utility
    std::function<void(NodeGraph&, glm::f32mat4x4&)> print_node_graph 
        = [&](NodeGraph& node, glm::f32mat4x4& parent_matrix){
            
        glm::f32mat4x4 transform = parent_matrix * node.model_matrix;
        //glm::f32mat4x4 transform = glm::f32mat4x4(1.0);
        
        for(auto& instance_desc_index : node.render_instance_desc_indices){
            
            auto& instance_desc = model_data.instance_descs[instance_desc_index];
            
            auto& raw_vertex_b        = model_data.raw_vertex_buffers[instance_desc.raw_vertex_buffer_idx];
            auto& raw_index_b         = model_data.raw_index_buffers[instance_desc.raw_index_buffer_idx];
            //.material_id_base       = material_id_base,
            //.material_id            = (instance_desc.material_desc_idx >= 0) ? instance_desc.material_desc_idx : (int32_t(default_material_id) - int32_t(material_id_base)),
            //.local_model_matrix     = transform,

            auto v_buffer = Irori::Core::UniqueBuffer::download(vulkan_context, 1024*1024);
            auto i_buffer = Irori::Core::UniqueBuffer::download(vulkan_context, 1024*1024);
            Irori::Core::Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){

                auto buffer_copy_info = vk::BufferCopy()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(raw_vertex_b.buffer_size);

                cb.copyBuffer(raw_vertex_b.get_raw_buffer(), v_buffer.get_raw_buffer(), {buffer_copy_info});

                buffer_copy_info = vk::BufferCopy()
                    .setSrcOffset(0)
                    .setDstOffset(0)
                    .setSize(raw_index_b.buffer_size);

                cb.copyBuffer(raw_index_b.get_raw_buffer(), i_buffer.get_raw_buffer(), {buffer_copy_info});
            });
            
            printf("%s\n", node.name.c_str());
            for(int i=0; i<instance_desc.raw_index_num; i++){
                int idx = ((int *)i_buffer.get_mapped_addr_for_read())[i];
                float v0 = ((Irori::Render::VertexType0*)v_buffer.get_mapped_addr_for_read())[idx].position[0];
                float v1 = ((Irori::Render::VertexType0*)v_buffer.get_mapped_addr_for_read())[idx].position[1];
                float v2 = ((Irori::Render::VertexType0*)v_buffer.get_mapped_addr_for_read())[idx].position[2];
                
                glm::vec4 v = {v0, v1, v2, 1.f};
                glm::vec4 vv = transform*v;
                //glm::vec4 vv = v*transform;
                printf("%d: %f %f %f\n",idx, vv[0], vv[1], vv[2]);
            }
            printf("\n");
        }

        
        // traverse chileren
        for(auto& child_node : node.children){
            print_node_graph(child_node, transform);
        }
    };
    
    auto& node_graph_root = models_data[model_name].node_graph_root;
    print_node_graph(node_graph_root, node_graph_root.model_matrix);
}
   
}











