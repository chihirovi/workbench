#pragma once

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
//#define TINYGLTF_NO_EXTERNAL_IMAGE //imageを読み込みたくないときに
#include <tinygltf/tiny_gltf.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
            
#include <functional>
#include <ranges>

#include "../../Source/Utility/Error.hpp"
#include "../../Source/Render/Cluster.hpp"
#include <random>
#include <iostream>

void load_vertex_index(
    tinygltf::Model& model, 
    tinygltf::Primitive& primitive,
    std::vector<glm::f32vec3>& positions,
    std::vector<glm::f32vec3>& normals,
    std::vector<glm::f32vec4>& tangents,
    std::vector<glm::u8vec4>& colors,
    std::vector<glm::f32vec2>& texcoords,
    std::vector<uint32_t>& indices){
   
    /*utilitys*/
    auto check_attribute_accessor = [&](char* name, int type, int component_type){
        ASSERT_WITH_MSG(primitive.attributes.contains(name), std::format("primitive dosen't containts {}", name));
        auto& accessor = model.accessors[primitive.attributes[name]];
        ASSERT_WITH_MSG((accessor.componentType == component_type), std::format("{} : worng component type [{}], expect [{}]", name, accessor.componentType, component_type));
        ASSERT_WITH_MSG((accessor.type == type), std::format("{} : worng type [{}], expect [{}]", name, accessor.type, type));
        return accessor;
    };
    
    auto data_begin_point = [&](tinygltf::Accessor& accessor){
        auto& buffer_view = model.bufferViews[accessor.bufferView];
        auto& buffer = model.buffers[buffer_view.buffer];
        
        return std::tuple<void*, size_t>{
            (void *)(&buffer.data[buffer_view.byteOffset + accessor.byteOffset]),
            buffer_view.byteStride
        };
    };
    
    auto read_vec_n = [&]<typename SrcT, typename DstT>(tinygltf::Accessor& accessor, uint32_t element_offset, uint32_t vec_n, DstT* p_dst_T){

        auto [p_void, byte_stride] = data_begin_point(accessor);

        // tinygltfではbyte strideは0になりうるらしい
        byte_stride = byte_stride>0 ? byte_stride : sizeof(SrcT) * vec_n;

        uint8_t* p_u8           = (uint8_t*)p_void;
        p_u8                    += byte_stride*element_offset;

        SrcT* p_src_T           = (SrcT*)p_u8;
        for(int i=0; i<vec_n; i++){
            p_dst_T[i] = (DstT)(p_src_T[i]);
        }
    };
    
    ASSERT_WITH_MSG((primitive.mode == TINYGLTF_MODE_TRIANGLES),"Irori only support triangle mode : triangle lists");
    
    // position
    auto position_accessor = check_attribute_accessor("POSITION", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
    positions.resize(position_accessor.count);

    for(int i=0; i<positions.size(); i++){
        read_vec_n.operator()<float, float>(position_accessor, i, 3, &positions[i][0]);
    }
    
    // normal
    if(primitive.attributes.contains("NORMAL")){

        normals.resize(positions.size());

        auto& normal_accessor = model.accessors[primitive.attributes["NORMAL"]];

        if(normal_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){

            check_attribute_accessor("NORMAL", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
               
            for(int i=0; i<normals.size(); i++){
                read_vec_n.operator()<float, float>(normal_accessor, i, 3, &normals[i][0]);
            }
        }
    }
    
    // texcoord_0, texcoord_1以上の複数のUVを使用するモデルはサポートしない
    if(primitive.attributes.contains("TEXCOORD_0")){

        texcoords.resize(positions.size());

        auto uv_accessor = check_attribute_accessor("TEXCOORD_0", TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT);

        for(int i=0; i<texcoords.size(); i++){
            read_vec_n.operator()<float, float>(uv_accessor, i, 2, &texcoords[i][0]);
        }   
    }
    
    // color
    if(primitive.attributes.contains("COLOR_0")){
        colors.resize(positions.size());
        auto& color_accessor = model.accessors[primitive.attributes["COLOR_0"]];

        if(color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE){

            check_attribute_accessor("COLOR_0", TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE);
            
            for(int i=0; i<colors.size(); i++){
                read_vec_n.operator()<uint8_t, uint8_t>(color_accessor, i, 4, &colors[i][0]);
            }   
            
        }else if(color_accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT){

            check_attribute_accessor("COLOR_0", TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT);
            
            for(int i=0; i<colors.size(); i++){
                float c_temp[4] = {1.0, 1.0, 1.0, 1.0};
                read_vec_n.operator()<float, float>(color_accessor, i, 3, c_temp);
                
                colors[i][0] = uint8_t(c_temp[0]*255);
                colors[i][1] = uint8_t(c_temp[1]*255);
                colors[i][2] = uint8_t(c_temp[2]*255);
                colors[i][3] = uint8_t(c_temp[3]*255);
            }   
        }
    }
    
    // index
    auto index_accessor = model.accessors[primitive.indices];
    ASSERT_WITH_MSG((index_accessor.count > 0), std::format("Irori only support indexed triangle"));
    ASSERT_WITH_MSG((index_accessor.type == TINYGLTF_TYPE_SCALAR), "index type must be SCALAR");
    indices.resize(index_accessor.count);

    if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT){
        for(int i=0; i<indices.size(); i++){
            read_vec_n.operator()<uint16_t, uint32_t>(index_accessor, i, 1, &indices[i]);
        }
        
    }else if(index_accessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT){
        for(int i=0; i<indices.size(); i++){
            read_vec_n.operator()<uint32_t, uint32_t>(index_accessor, i, 1, &indices[i]);
        }
    }
    
    // tangent
    if(primitive.attributes.contains("TANGENT")){
        auto tangent_accessor = check_attribute_accessor("TANGENT", TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT);
        tangents.resize(positions.size());

        for(int i=0; i<tangents.size(); i++){
            read_vec_n.operator()<float, float>(tangent_accessor, i, 4, &tangents[i][0]);
        }   
    }
}

std::string randomColor(){
    static std::mt19937 rng(12345);
    static std::uniform_real_distribution<float> dist(0.2f, 1.0f);
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(3)
       << dist(rng) << " " << dist(rng) << " " << dist(rng);
    return ss.str(); // "r g b"
}

void exportClustersToOBJ(
    std::vector<Irori::Render::Cluster>& clusters,
    std::vector<glm::f32vec3>& verts,
    std::vector<uint32_t>& c_verts,
    std::vector<uint8_t>& c_idxs,
    std::string objFilename,
    uint32_t lod){

    std::string mtlFilename = objFilename.substr(0, objFilename.find_last_of('.')) + ".mtl";

    std::ofstream obj(objFilename, std::ios::out | std::ios::trunc);
    std::ofstream mtl(mtlFilename, std::ios::out | std::ios::trunc);

    if (!obj || !mtl) {
        printf("can't open file\n");
        return;
    }

    obj << "# Clustered mesh export\n";
    obj << "mtllib " << mtlFilename.substr(mtlFilename.find_last_of("/\\") + 1) << "\n\n";

    
    obj << "o object" << "\n";
    // --- OBJに頂点出力 ---
    for (auto& v : verts)
        obj << "v " << v.x << " " << v.y << " " << v.z << "\n";
    obj << "\n";
    for (size_t c = 0; c < clusters.size(); ++c) {
        auto& cluster = clusters[c];
        if(cluster.lod_level != lod) continue;
        std::string materialName = "cluster_" + std::to_string(c);

        // --- MTL にマテリアル定義を追加 ---
        mtl << "newmtl " << materialName << "\n";
        mtl << "Kd " << randomColor() << "\n"; // 拡散反射色
        mtl << "Ka 0.0 0.0 0.0\n";
        mtl << "Ks 0.0 0.0 0.0\n";
        mtl << "d 1.0\n\n";


        // --- マテリアル切り替え ---
        obj << "usemtl " << materialName << "\n";
        //obj << "g cluster_" << c << "\n";

        // --- 面の出力（インデックスは1始まり、頂点は全体通し） ---
        for (size_t i = 0; i < cluster.triangle_count; i++) {
            uint32_t i0 = c_verts[ cluster.vertex_offset + c_idxs[cluster.triangle_offset + i*3 + 0] ];
            uint32_t i1 = c_verts[ cluster.vertex_offset + c_idxs[cluster.triangle_offset + i*3 + 1] ];
            uint32_t i2 = c_verts[ cluster.vertex_offset + c_idxs[cluster.triangle_offset + i*3 + 2] ];
            obj << "f " << (i0+1) << " " << (i1+1) << " " << (i2+1) << "\n";
        }

        obj << "\n";
    }

    std::cout << "OBJとMTLを出力しました: " << objFilename << " / " << mtlFilename << "\n";
}