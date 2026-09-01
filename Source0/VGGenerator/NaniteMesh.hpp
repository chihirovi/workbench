#pragma once

#include "../../Source/Render/Cluster.hpp"
#include "../../External/meshoptimizer/src/meshoptimizer.h"
#include "Helper.hpp"
#include <vector>
#include <ranges>

#define MAX_LOD 8

void generate_nanite_mesh(
    std::vector<Irori::Render::Cluster>& lod0_clusters, 
    std::vector<glm::f32vec3>& positions,
    std::vector<uint32_t>& lod0_cluster_vertexs,
    std::vector<uint8_t>& lod0_cluster_triangles){
    printf("%d\n", lod0_clusters.size());
    
    // init, generate bound sphere
    for(auto& c : lod0_clusters){
        meshopt_Bounds bounds = meshopt_computeMeshletBounds(
            &lod0_cluster_vertexs[c.vertex_offset], 
            &lod0_cluster_triangles[c.triangle_offset],
            c.triangle_count, 
            &positions[0].x, 
            positions.size(), 
            sizeof(glm::f32vec3)
        );
        c.self_lod_bound_sphere[0] = bounds.center[0];
        c.self_lod_bound_sphere[1] = bounds.center[1];
        c.self_lod_bound_sphere[2] = bounds.center[2];
        c.self_lod_bound_sphere[3] = bounds.radius;
    }
    
    // 
    std::vector<Irori::Render::Cluster> lod_clusters[MAX_LOD]; 
    std::vector<uint32_t> lod_vertexs[MAX_LOD]; 
    std::vector<uint8_t> lod_triangles[MAX_LOD]; 
    
    uint32_t meshlet_vertex_sum = 0;
    uint32_t meshlet_triangle_sum = 0;

    for(int lod = 1; lod<=MAX_LOD; lod++){
        
        std::vector<Irori::Render::Cluster>* pre_lod_clusters;
        std::vector<uint32_t>* pre_lod_vertexs;
        std::vector<uint8_t>* pre_lod_triangles;
        if(lod == 1){
            pre_lod_clusters = &lod0_clusters;
            pre_lod_vertexs = &lod0_cluster_vertexs;
            pre_lod_triangles = &lod0_cluster_triangles;
        }else{
            pre_lod_clusters = &lod_clusters[lod-2];
            pre_lod_vertexs = &lod_vertexs[lod-2];
            pre_lod_triangles = &lod_triangles[lod-2];
        }
        
        // partition
        std::vector<uint32_t> vertex_index;
        std::vector<uint32_t> cluster_index_counts;
        for(auto& cluster : *pre_lod_clusters){
            for(int i=0; i<cluster.triangle_count*3; i++){
                vertex_index.push_back((*pre_lod_vertexs)[cluster.vertex_offset-meshlet_vertex_sum + (*pre_lod_triangles)[cluster.triangle_offset- meshlet_triangle_sum + i]]);
            }
            cluster_index_counts.push_back(cluster.triangle_count*3);
        }

        uint32_t partition_size = 4;
        std::vector<unsigned int> cluster_partitions(pre_lod_clusters->size());
        size_t partition_count = meshopt_partitionClusters(
            &cluster_partitions[0], 
            &vertex_index[0], 
            vertex_index.size(),
            &cluster_index_counts[0], 
            pre_lod_clusters->size(), 
            &positions[0].x, 
            positions.size(), 
            sizeof(glm::f32vec3), 
            partition_size
        );

        /*
        for(auto& i : cluster_partitions){
            printf("%d ", i);
        }*/

        // marge
        std::vector<std::vector<uint32_t>> groups(partition_count);
        std::vector<std::vector<Irori::Render::Cluster*>> children(partition_count);
        for(int group = 0; group < partition_count; group++){
            
            for(int i=0; i<pre_lod_clusters->size(); i++){
                if(cluster_partitions[i] != group) continue;
                
                auto& cluster = (*pre_lod_clusters)[i];
                
                for(int v=0; v<cluster.triangle_count; v++){
                    groups[group].push_back((*pre_lod_vertexs)[cluster.vertex_offset - meshlet_vertex_sum + (*pre_lod_triangles)[cluster.triangle_offset - meshlet_triangle_sum + v*3 + 0]]);                
                    groups[group].push_back((*pre_lod_vertexs)[cluster.vertex_offset - meshlet_vertex_sum + (*pre_lod_triangles)[cluster.triangle_offset - meshlet_triangle_sum + v*3 + 1]]);                
                    groups[group].push_back((*pre_lod_vertexs)[cluster.vertex_offset - meshlet_vertex_sum + (*pre_lod_triangles)[cluster.triangle_offset - meshlet_triangle_sum + v*3 + 2]]);                
                }

                children[group].push_back(&cluster);
            }
        }

        // simpligy
        std::vector<float> groups_error(partition_count);
        for(const auto& [group_id, group] : groups|std::views::enumerate){

            size_t target_index_count = std::max<size_t>(3, group.size()/2);
            std::vector<uint32_t> simplified_indices(group.size());
            float lod_error;
            simplified_indices.resize(meshopt_simplify(
                simplified_indices.data(), 
                group.data(), 
                group.size(), 
                &positions[0].x, 
                positions.size(), 
                sizeof(glm::f32vec3), 
                target_index_count, 
                0.01*lod, 
                meshopt_SimplifyLockBorder, 
                &lod_error
            ));


            for(int i = 0; i<simplified_indices.size(); i++){
                group[i] = simplified_indices[i];
            }
            group.resize(simplified_indices.size());
            
            groups_error[group_id] = lod_error;

            {// compute simplify scale.
                /*
                std::vector simplifyIndices;
                simplifyIndices.resize(optimizedIndices.size());
                size_t numIndices = meshopt_simplify(
                    simplifyIndices.data(),
                    optimizedIndices.data(), 
                    optimizedIndices.size(),
                    reinterpret_cast(InPositions.data()), 
                    InPositions.size(), 
                    sizeof(Vector3),
                    optimizedIndices.size() / 2, 
                    0.05f, 
                    meshopt_SimplifyLockBorder, 
                    &OutError
                );*/
                // compute simplify scale.
                std::vector<uint32_t> remap;
                remap.resize(positions.size());//remap.resize(InPositions.size());
                size_t numRemapVertices = meshopt_generateVertexRemap(
                    remap.data(), 
                    group.data(),   //optimizedIndices.data(), 
                    group.size(),   //optimizedIndices.size(),
                    positions.data(), 
                    positions.size(), 
                    sizeof(glm::f32vec3)
                );
                std::vector<glm::f32vec3> remapPositions;
                remapPositions.resize(numRemapVertices);
                meshopt_remapVertexBuffer(
                    remapPositions.data(), 
                    positions.data(), 
                    positions.size(), 
                    sizeof(glm::f32vec3), 
                    remap.data()
                );
                //float localScale = meshopt_simplifyScale(reinterpret_cast(remapPositions.data()), remapPositions.size(), sizeof(Vector3));
                float localScale = meshopt_simplifyScale(
                    &remapPositions[0].x, 
                    remapPositions.size(), 
                    sizeof(glm::f32vec3)
                );

                groups_error[group_id] *= localScale;
            }
        }

        // カウントを増やすタイミングはここ
        meshlet_vertex_sum += pre_lod_vertexs->size();
        meshlet_triangle_sum += pre_lod_triangles->size();

        // split
        uint32_t group_meshlet_vertex_offset = 0;
        uint32_t group_meshlet_triangle_offset = 0;
        std::vector<std::vector<uint32_t>> parents(partition_count);
        for(const auto& [group_id, group] : groups|std::views::enumerate){

            size_t temp_maxMeshlets = meshopt_buildMeshletsBound(group.size(), CLUSTER_MAX_VERTEX, CLUSTER_MAX_TRIANGLE);
    
            std::vector<meshopt_Meshlet> temp_meshlets(temp_maxMeshlets);
            std::vector<uint32_t> temp_meshletVertices(temp_maxMeshlets * CLUSTER_MAX_VERTEX);
            std::vector<uint8_t> temp_meshletTriangles(group.size() + temp_maxMeshlets * 3);
            
            // メッシュレット構築
            size_t temp_meshletCount = meshopt_buildMeshlets(
                temp_meshlets.data(),
                temp_meshletVertices.data(),
                temp_meshletTriangles.data(),
                group.data(),
                group.size(),
                &positions[0].x,
                positions.size(),
                sizeof(glm::f32vec3),
                CLUSTER_MAX_VERTEX,     // 最大頂点数
                CLUSTER_MAX_TRIANGLE,   // 最大三角形数
                0.0f                    // エラー許容（0で完全分割）
            );
            
            temp_meshlets.resize(temp_meshletCount);
            temp_meshletVertices.resize(temp_meshlets.back().vertex_offset + temp_meshlets.back().vertex_count);
            temp_meshletTriangles.resize(temp_meshlets.back().triangle_offset + temp_meshlets.back().triangle_count*3);

            std::vector<Irori::Render::Cluster> temp_irori_clusters(temp_meshlets.size());

            for(int i=0; i<temp_meshlets.size(); i++){
                temp_irori_clusters[i].vertex_offset    = temp_meshlets[i].vertex_offset + group_meshlet_vertex_offset + meshlet_vertex_sum;
                temp_irori_clusters[i].triangle_offset  = temp_meshlets[i].triangle_offset + group_meshlet_triangle_offset + meshlet_triangle_sum;
                temp_irori_clusters[i].vertex_count     = temp_meshlets[i].vertex_count;
                temp_irori_clusters[i].triangle_count   = temp_meshlets[i].triangle_count;
                temp_irori_clusters[i].lod_level        = lod;
                //temp_irori_clusters[i].error_self       = 0.0f;
                temp_irori_clusters[i].error_parent     = -1.0f;
                //irori_clusters[i].self_lod_bound_sphere = ;
                //irori_clusters[i].parent_lod_bound_sphere;
            }

            group_meshlet_vertex_offset += temp_meshletVertices.size();
            group_meshlet_triangle_offset += temp_meshletTriangles.size();
            
            for(auto& tc : temp_irori_clusters){
                lod_clusters[lod-1].push_back(tc);
                parents[group_id].push_back(lod_clusters[lod-1].size()-1);
            }
            for(auto& v : temp_meshletVertices){
                lod_vertexs[lod-1].push_back(v);
            }
            for(auto& i : temp_meshletTriangles){
                lod_triangles[lod-1].push_back(i);
            }
        }
        

        // calc error
        for(int group_id = 0; group_id < partition_count; group_id++){

            // error
            float child_error = 0.0;
            for(auto& cc : children[group_id]) child_error = std::max(child_error, cc->error_self);
            for(auto& pc : parents[group_id])  lod_clusters[lod-1][pc].error_self = groups_error[group_id] + child_error;
            for(auto& cc : children[group_id]) cc->error_parent = groups_error[group_id] + child_error;
            

            // error sphere
            std::vector<glm::f32vec3> centers;
            std::vector<float> radii;
            for(auto& cc : children[group_id]){
                centers.push_back({cc->self_lod_bound_sphere[0], cc->self_lod_bound_sphere[1], cc->self_lod_bound_sphere[2]});
                radii.push_back(cc->self_lod_bound_sphere[3]);
            }


            meshopt_Bounds merged = meshopt_computeSphereBounds(
                &centers[0].x, 
                centers.size(), 
                sizeof(glm::f32vec3), 
                radii.data(),
                sizeof(float)
            );
            

            for(auto& cc : children[group_id]){
                cc->parent_lod_bound_sphere[0] = merged.center[0];
                cc->parent_lod_bound_sphere[1] = merged.center[1];
                cc->parent_lod_bound_sphere[2] = merged.center[2];
                cc->parent_lod_bound_sphere[3] = merged.radius;
            }
            for(auto& pc : parents[group_id]){
                lod_clusters[lod-1][pc].self_lod_bound_sphere[0] = merged.center[0];
                lod_clusters[lod-1][pc].self_lod_bound_sphere[1] = merged.center[1];
                lod_clusters[lod-1][pc].self_lod_bound_sphere[2] = merged.center[2];
                lod_clusters[lod-1][pc].self_lod_bound_sphere[3] = merged.radius;
            }
            

            // 末端処理
            for(auto& pc : parents[group_id]){
                lod_clusters[lod-1][pc].error_parent = INFINITY;
                lod_clusters[lod-1][pc].parent_lod_bound_sphere[0] = 0.0;
                lod_clusters[lod-1][pc].parent_lod_bound_sphere[1] = 0.0;
                lod_clusters[lod-1][pc].parent_lod_bound_sphere[2] = 0.0;
                lod_clusters[lod-1][pc].parent_lod_bound_sphere[3] = INFINITY;
            }

            
        }


    }
    
    // push back
    for(int lod = 1; lod<=MAX_LOD; lod++){
        for(auto& c : lod_clusters[lod-1]){
            lod0_clusters.push_back(c);
        }
        for(auto& v : lod_vertexs[lod-1]){
            lod0_cluster_vertexs.push_back(v);
        }
        for(auto& i : lod_triangles[lod-1]){
            lod0_cluster_triangles.push_back(i);
        }
    }
}









