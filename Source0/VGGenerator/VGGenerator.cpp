#include "Helper.hpp"
#include "NaniteMesh.hpp"
#include "../../Source/Render/Cluster.hpp"
#include "../../External/meshoptimizer/src/meshoptimizer.h"
#include "../../Source/VirtualGeometry/VG_File.hpp"
#include <iostream>

// ./vg_generator.exe ../../Assets/Bunny/bunny.gltf ../../Assets/Bunny/bunny.vg
int main(int argc, char** argv){
    
    if(argc != 3){
        printf("Usage : input_file output_file\n");
        return 1;
    }
    
    std::string model_path = argv[1];
    std::string vg_path = argv[2];
    
    // load original model
    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;
    bool ret = loader.LoadASCIIFromFile(&model, &err, &warn, model_path);
    if (!warn.empty()) std::cout<< "Warning: " << warn << "\n";
    if (!err.empty()) std::cout << "Error: " << err << "\n";
    if (!ret) {
        std::cout<< "Failed to load glTF: " << model_path << "\n";
        return 1;
    }


    auto& primitive = model.meshes[0].primitives[0];

    // load geometry data
    std::vector<glm::f32vec3> positions;
    std::vector<glm::f32vec3> normals;
    std::vector<glm::f32vec4> tangents;
    std::vector<glm::u8vec4> colors;
    std::vector<glm::f32vec2> texcoords;
    std::vector<uint32_t> indices;
    load_vertex_index(model, primitive, positions, normals, tangents, colors, texcoords, indices);
    
    size_t maxMeshlets = meshopt_buildMeshletsBound(indices.size(), CLUSTER_MAX_VERTEX, CLUSTER_MAX_TRIANGLE);
    
    std::vector<meshopt_Meshlet> meshlets(maxMeshlets);
    std::vector<uint32_t> meshletVertices(maxMeshlets * CLUSTER_MAX_VERTEX);
    std::vector<uint8_t> meshletTriangles(indices.size() + maxMeshlets * 3);
    
    // メッシュレット構築
    size_t meshletCount = meshopt_buildMeshlets(
        meshlets.data(),
        meshletVertices.data(),
        meshletTriangles.data(),
        indices.data(),
        indices.size(),
        &positions[0].x,
        positions.size(),
        sizeof(glm::f32vec3),
        CLUSTER_MAX_VERTEX,     // 最大頂点数
        CLUSTER_MAX_TRIANGLE,   // 最大三角形数
        0.0f                    // エラー許容（0で完全分割）
    );
    
    meshlets.resize(meshletCount);
    meshletVertices.resize(meshlets.back().vertex_offset + meshlets.back().vertex_count);
    meshletTriangles.resize(meshlets.back().triangle_offset + meshlets.back().triangle_count*3);

    std::vector<Irori::Render::Cluster> irori_clusters(meshlets.size());
    for(int i=0; i<meshlets.size(); i++){
        irori_clusters[i].vertex_offset = meshlets[i].vertex_offset;
        irori_clusters[i].triangle_offset = meshlets[i].triangle_offset;
        irori_clusters[i].vertex_count = meshlets[i].vertex_count;
        irori_clusters[i].triangle_count = meshlets[i].triangle_count;
        irori_clusters[i].lod_level = 0;
        irori_clusters[i].error_self = 0.0f;
        irori_clusters[i].error_parent = -1.0;
        //irori_clusters[i].self_lod_bound_sphere = ;
        //irori_clusters[i].parent_lod_bound_sphere;
    }
    
    
    generate_nanite_mesh(irori_clusters, positions, meshletVertices, meshletTriangles);
    
    //calc cluster aabb
    for(auto& cluster : irori_clusters){

        glm::f32vec3 aabb_min = {FLT_MAX, FLT_MAX, FLT_MAX};
        glm::f32vec3 aabb_max = {-FLT_MAX, -FLT_MAX, -FLT_MAX};

        for(int vertex_idx = 0; vertex_idx < cluster.vertex_count; vertex_idx++){
            int vi = meshletVertices[cluster.vertex_offset + vertex_idx];
            
            const auto& v = positions[vi];
            
            aabb_min = glm::min(aabb_min, v);
            aabb_max = glm::max(aabb_max, v);
        }

        // aabb_min
        cluster.self_aabb[0] = aabb_min[0];
        cluster.self_aabb[1] = aabb_min[1];
        cluster.self_aabb[2] = aabb_min[2];

        // aabb_max
        cluster.self_aabb[3] = aabb_max[0];
        cluster.self_aabb[4] = aabb_max[1];
        cluster.self_aabb[5] = aabb_max[2];
    }
    
    
    Write_VG_File(vg_path, irori_clusters, meshletVertices, meshletTriangles);
    {
        std::vector<Irori::Render::Cluster> clusters;
        std::vector<uint32_t> verts;
        std::vector<uint8_t> idxs;
        Read_VG_File(vg_path, clusters, verts, idxs);
        exportClustersToOBJ(clusters, positions, verts, idxs, "vg_test.obj", 4);
    }
    
}