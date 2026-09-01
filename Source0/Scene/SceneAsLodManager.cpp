#include "SceneAsLodManager.hpp"

namespace Irori::Scene{

void UniqueSceneAsLodManager::rebuild_tlas_blas(UniqueSceneManager& scene_manager, Render::CPU_FrameRenderData& cpu_frame_render_data, SceneAsModelLodFile& lod_files){
    
    // create blas_s
    lod_blas.resize(0);
    model_name_2_lod_vi_idx.clear();
    
    // lod0
    //lod_blas_s[0].resize(0);
    //rt_render_instance_global_id_map.resize(0);
    model_2_blas_idx_RT_render_inst_base_map.clear();
    uint64_t blas_submesh_count = 0;
    for(auto& [model_name, render_instance_range] : scene_manager.model_name_to_render_inst_table){

        uint32_t render_instance_id_base = render_instance_range.first;
        uint32_t render_instance_count = render_instance_range.second;
        blas_submesh_count += render_instance_count;

        // set rt render instance data
        std::vector<RayTrace::BlasSubMeshInfo> blas_create_info;

        for(int i=0; i<render_instance_count; i++){

            uint32_t render_instance_global_id = render_instance_id_base + i;

            auto& render_instance = cpu_frame_render_data.render_instance_table[render_instance_global_id];

            RayTrace::BlasSubMeshInfo blas_sub_mesh_info{
                .vertex_address = render_instance.raw_vertex_addr,
                .index_address  = render_instance.raw_index_addr,
                .vertex_stride  = sizeof(Render::VertexType0),
                .vertex_num     = render_instance.raw_vertex_num,
                .index_num      = render_instance.raw_triangle_num*3,
                .vertex_format  = vk::Format::eR32G32B32Sfloat,
                .index_type     = vk::IndexType::eUint32,
                .transform      = render_instance.local_model_matrix,
                //.geometry_frag  = vk::GeometryFlagBitsKHR::eOpaque,
                //.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation,// any hit ならこれを使う
            };

            uint32_t blend_mode = cpu_frame_render_data.material_table[render_instance.material_id_base + render_instance.material_id].blend_mode;

            if(blend_mode == Render::MaterialBlendMode::opaque){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

            }else if(blend_mode == Render::MaterialBlendMode::mask){
                //blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;

            }else if(blend_mode == Render::MaterialBlendMode::blend){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

            }else{
                ASSERT_WITH_MSG(false, "unknown material blend mode");          
            }
            
            //rt_render_instance_global_id_map.push_back(render_instance_global_id);
            blas_create_info.push_back(blas_sub_mesh_info);
        }
        
        // create blas
        //lod_blas_s[0].push_back(std::move(RayTrace::create_blas(vulkan_context, blas_create_info)));
        lod_blas.push_back(std::move(RayTrace::create_blas(vulkan_context, blas_create_info)));

        // register
        model_2_blas_idx_RT_render_inst_base_map[model_name] = {
            //lod_blas_s[0].size()-1, 
            lod_blas.size()-1, 
            render_instance_id_base
        };
    }

    // build tlas
    std::vector<RayTrace::TlasInstanceInfo> tlas_create_info;
    tlas_instance_count = 0;
    for(auto& [model_name, model_instances] : scene_manager.models_instances){

        ASSERT_WITH_MSG((model_2_blas_idx_RT_render_inst_base_map.contains(model_name)), std::format("model 2 blas_idx render inst base dosen't containt model name [{}].", model_name));
        
        auto& tmp = model_2_blas_idx_RT_render_inst_base_map[model_name];
        auto blas_idx = tmp.first;
        auto rt_render_inst_base = tmp.second;

        for(auto& [modol_inst_name, model_inst] : model_instances){

            tlas_create_info.push_back(RayTrace::TlasInstanceInfo{
                .blas_index = blas_idx,
                .transform = model_inst.transform,
                .instance_custom_index = rt_render_inst_base,
            });

            tlas_instance_count++;
        }
    }
    
    //lod_tlas[0] = std::move(RayTrace::create_tlas(vulkan_context, tlas_create_info, lod_blas_s[0]));
    lod_tlas[0] = std::move(RayTrace::create_tlas(vulkan_context, tlas_create_info, lod_blas));
    
    // tlal instances data buffer
    lod_tlas_instances_data_buffer[0] = Core::UniqueBuffer::common(
        vulkan_context,
        tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR),
        false,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
        vk::BufferUsageFlagBits::eShaderDeviceAddress|
        vk::BufferUsageFlagBits::eTransferDst,
        16
    );   

    lod_tlas_instances_data_staging_buffer[0] = Core::UniqueBuffer::upload(
        vulkan_context,
        tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR)
    );  
    
    ////////////////////////////////////
    ////////////////////////////////////
    // check lod file name
    for(auto& [model_name, _] : scene_manager.model_name_to_render_inst_table){
        ASSERT_WITH_MSG(lod_files.model_name_2_lod_files.contains(model_name), std::format("lod files dosen't contain model name [{}].", model_name));
        ASSERT_WITH_MSG(lod_files.model_name_2_lod_files[model_name][0] != std::nullopt, std::format("model [{}] must have lod 1 file name.", model_name));
        /*
        for(int i=1; i<=3; i++){
            if(lod_files.model_name_2_lod_files[model_name][i] == std::nullopt){
                lod_files.model_name_2_lod_files[model_name][i] = lod_files.model_name_2_lod_files[model_name][i-1];
            }            
        }*/
    }
    
    //build lod 1 2 3 4 as
    vertex_buffers.resize(0);
    index_buffers.resize(0);
    rt_lod1234_vert_idx_addr.resize(0);
    rt_lod1234_vert_idx_addr.push_back(blas_submesh_count);

    for(int lod = 1; lod <= 4; lod++){
        // blas s
        // lod_blas_s[lod].resize(0);

        // scene_manager.model_name_to_render_inst_table の読み出し順番がlod0の時と同じだから
        // lod_blas_s[lod]に入るblasとそのsub_meshの順番も揃う
        // つまり，model_nameとblas_idxの対応関係はlod0,1,2,3,4で同じになる
        // なので model_2_blas_idx_RT_render_inst_base_map[model_name]， re_render_intance_global_id_mapを使いまわせる
        for(auto& [model_name, render_instance_range] : scene_manager.model_name_to_render_inst_table){

            uint32_t render_instance_id_base = render_instance_range.first;
            uint32_t render_instance_count = render_instance_range.second;


            // load model 
            if( lod_files.model_name_2_lod_files[model_name][lod-1]){

                std::string lod_file_name = lod_files.model_name_2_lod_files[model_name][lod-1].value();
                // load gltf model
                tinygltf::Model gltf_model;
                tinygltf::TinyGLTF loader;
                std::string err, warn;

                tinygltf::LoadImageDataFunction dummy_image_loader = [](
                    tinygltf::Image* _0, const int _1, std::string* _2, std::string* _3, int _4, int _5, const unsigned char* _6, int _7, void* _8) -> bool {
                    return true;
                };

                loader.SetImageLoader(dummy_image_loader, nullptr);
                ASSERT_WITH_MSG((loader.LoadASCIIFromFile(
                    &gltf_model, 
                    &err, 
                    &warn, 
                    lod_file_name 
                )), std::format("load gltf file error {}", err));
                //if(warn.size() != 0) printf("load gltf file warn : %s\n", warn.c_str());

                // set rt render instance data
                std::vector<RayTrace::BlasSubMeshInfo> blas_create_info;

                for(int i=0; i<render_instance_count; i++){

                    uint32_t render_instance_global_id = render_instance_id_base + i;

                    auto& render_instance = cpu_frame_render_data.render_instance_table[render_instance_global_id];
                    auto [mesh_id, prim_id] = scene_manager.render_inst_table_gltf_mesh_prim_id_map[render_instance_global_id];
                    
                    auto& primitive = gltf_model.meshes[mesh_id].primitives[prim_id];
                    std::vector<Irori::Render::VertexType0> verts;
                    std::vector<uint32_t> idxs;

                    load_vertex_index(gltf_model, primitive, verts, idxs);

                    auto vertex_buffer = Core::UniqueBuffer::common(
                        vulkan_context, 
                        sizeof(Render::VertexType0)*verts.size(), 
                        false, 
                        vk::BufferUsageFlagBits::eStorageBuffer|
                        vk::BufferUsageFlagBits::eVertexBuffer|
                        vk::BufferUsageFlagBits::eShaderDeviceAddress|
                        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
                        vk::BufferUsageFlagBits::eTransferDst, 
                        8
                    );
                    vertex_buffer.transfer_data(verts.data(), sizeof(Render::VertexType0)*verts.size());

                    auto index_buffer = Core::UniqueBuffer::common(
                        vulkan_context, 
                        sizeof(uint32_t)*idxs.size(), 
                        false, 
                        vk::BufferUsageFlagBits::eStorageBuffer|
                        vk::BufferUsageFlagBits::eIndexBuffer|
                        vk::BufferUsageFlagBits::eShaderDeviceAddress|
                        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
                        vk::BufferUsageFlagBits::eTransferDst, 
                        8
                    );
                    index_buffer.transfer_data(idxs.data(), sizeof(uint32_t)*idxs.size());
                    
                    vertex_buffers.push_back(std::move(vertex_buffer));
                    index_buffers.push_back(std::move(index_buffer));

                    RayTrace::BlasSubMeshInfo blas_sub_mesh_info{
                        .vertex_address = vertex_buffers.back().get_device_address(),
                        .index_address  = index_buffers.back().get_device_address(),
                        .vertex_stride  = sizeof(Render::VertexType0),
                        .vertex_num     = (uint32_t)verts.size(),
                        .index_num      = (uint32_t)idxs.size(),
                        .vertex_format  = vk::Format::eR32G32B32Sfloat,
                        .index_type     = vk::IndexType::eUint32,
                        .transform      = render_instance.local_model_matrix,
                        //.geometry_frag  = vk::GeometryFlagBitsKHR::eOpaque,
                        //.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation,// any hit ならこれを使う
                    };

                    uint32_t blend_mode = cpu_frame_render_data.material_table[render_instance.material_id_base + render_instance.material_id].blend_mode;

                    if(blend_mode == Render::MaterialBlendMode::opaque){
                        blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

                    }else if(blend_mode == Render::MaterialBlendMode::mask){
                        //blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;
                        blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;

                    }else if(blend_mode == Render::MaterialBlendMode::blend){
                        blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

                    }else{
                        ASSERT_WITH_MSG(false, "unknown material blend mode");          
                    }
                    
                    blas_create_info.push_back(blas_sub_mesh_info);
                    rt_lod1234_vert_idx_addr.push_back(vertex_buffers.back().get_device_address());
                    rt_lod1234_vert_idx_addr.push_back(index_buffers.back().get_device_address());
                    model_name_2_lod_vi_idx[model_name][lod-1].second.push_back(vertex_buffers.size()-1);
                }
                
                // create blas
                lod_blas.push_back(std::move(RayTrace::create_blas(vulkan_context, blas_create_info)));
                model_name_2_lod_vi_idx[model_name][lod-1].first = lod_blas.size()-1;
            
            }else{ // lod fileが全部ない場合は，ひとつ前のものをそのまま使う

                model_name_2_lod_vi_idx[model_name][lod-1] = model_name_2_lod_vi_idx[model_name][lod-2];
                for(auto& i : model_name_2_lod_vi_idx[model_name][lod-1].second){
                    rt_lod1234_vert_idx_addr.push_back(vertex_buffers[i].get_device_address());
                    rt_lod1234_vert_idx_addr.push_back(index_buffers[i].get_device_address());
                }
            }
        }

        // tlas
        std::vector<RayTrace::TlasInstanceInfo> tlas_create_info;
        for(auto& [model_name, model_instances] : scene_manager.models_instances){

            auto& tmp = model_2_blas_idx_RT_render_inst_base_map[model_name];
            auto rt_render_inst_base = tmp.second;

            for(auto& [modol_inst_name, model_inst] : model_instances){

                tlas_create_info.push_back(RayTrace::TlasInstanceInfo{
                    .blas_index = model_name_2_lod_vi_idx[model_name][lod-1].first,
                    .transform = model_inst.transform,
                    .instance_custom_index = rt_render_inst_base,
                });
            }
        }
        
        lod_tlas[lod] = std::move(RayTrace::create_tlas(vulkan_context, tlas_create_info, lod_blas));   

        // tlal instances data buffer
        lod_tlas_instances_data_buffer[lod] = Core::UniqueBuffer::common(
            vulkan_context,
            tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR),
            false,
            vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
            vk::BufferUsageFlagBits::eShaderDeviceAddress|
            vk::BufferUsageFlagBits::eTransferDst,
            16
        );   

        lod_tlas_instances_data_staging_buffer[lod] = Core::UniqueBuffer::upload(
            vulkan_context,
            tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR)
        );  
    }
}


void UniqueSceneAsLodManager::prepare_tlas_update_data(UniqueSceneManager& scene_manager){
    
    // prepare update data
    for(int lod = 0; lod <= 4; lod++){

        std::vector<vk::AccelerationStructureInstanceKHR> accel_instances;
        accel_instances.reserve(tlas_instance_count);

        for(auto& [model_name, model_instances] : scene_manager.models_instances){

            ASSERT_WITH_MSG((model_2_blas_idx_RT_render_inst_base_map.contains(model_name)), std::format("model 2 blas_idx render inst base dosen't containt model name [{}].", model_name));
            
            auto& tmp = model_2_blas_idx_RT_render_inst_base_map[model_name];
            auto rt_render_inst_base = tmp.second;
            
            uint32_t blas_idx;
            if(lod == 0)    blas_idx = tmp.first;
            else            blas_idx = model_name_2_lod_vi_idx[model_name][lod-1].first;

            for(auto& [modol_inst_name, model_inst] : model_instances){

                vk::TransformMatrixKHR transform = RayTrace::convert_transform(model_inst.transform); 

                auto accel_instance = vk::AccelerationStructureInstanceKHR()
                    .setTransform(transform)
                    .setInstanceCustomIndex(rt_render_inst_base)
                    .setMask(0xFFFFFFFF)
                    .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                    .setAccelerationStructureReference(lod_blas[blas_idx].get_AS_address())
                    .setInstanceShaderBindingTableRecordOffset(0);

                accel_instances.push_back(accel_instance);//ここの順番は，作成済みのtlasと揃えないとダメ
            }
        }
        
        // transfer to staging
        ASSERT_WITH_MSG((lod_tlas_instances_data_staging_buffer[lod].buffer != VK_NULL_HANDLE), "tlas instances data staging buffer is null handle.");
        
        lod_tlas_instances_data_staging_buffer[lod].transfer_data(accel_instances.data(), accel_instances.size()*sizeof(vk::AccelerationStructureInstanceKHR));

    }
}

void UniqueSceneAsLodManager::build_cpu_frame_render_data(Render::CPU_FrameRenderData* cpu_frame_render_data){
    cpu_frame_render_data->rt_lod1234_vert_idx_addr.insert(
        cpu_frame_render_data->rt_lod1234_vert_idx_addr.end(),
        rt_lod1234_vert_idx_addr.begin(),
        rt_lod1234_vert_idx_addr.end()
    );
}

void UniqueSceneAsLodManager::cmd_update_tlas_with_barrier(vk::CommandBuffer& cmd_buf){
    
    for(int lod=0; lod<=4; lod++){
        // copy data
        lod_tlas_instances_data_staging_buffer[lod].cmd_copy_to_buffer(
            cmd_buf,
            lod_tlas_instances_data_buffer[lod],
            tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR)
        );
        
        // barrier
        Core::CmdBufferBarrier::transfer_dst_2_as_update_read(cmd_buf, lod_tlas_instances_data_buffer[lod]);
        
        // update
        RayTrace::cmd_update_tlas(cmd_buf, lod_tlas[lod], tlas_instance_count, lod_tlas_instances_data_buffer[lod]);
        
        // memory barrier
        vk::MemoryBarrier2 barrier = vk::MemoryBarrier2()
            .setSrcStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR)
            .setSrcAccessMask(vk::AccessFlagBits2::eAccelerationStructureWriteKHR)
            .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
            .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR);

        vk::DependencyInfo depInfo = vk::DependencyInfo()
            .setMemoryBarriers(barrier);

        cmd_buf.pipelineBarrier2(depInfo);
    }
}

std::pair<uint32_t, uint32_t> UniqueSceneAsLodManager::get_all_AS_size(UniqueSceneManager& scene_manager){
    
    // lod0 as size
    uint32_t lod0_as_size = 0;
    for(auto& [model_name, _] : scene_manager.models_instances){
        uint32_t blas_idx = model_2_blas_idx_RT_render_inst_base_map[model_name].first;
        lod0_as_size += lod_blas[blas_idx].buffer_as.buffer_size;
    }   
    lod0_as_size += lod_tlas[0].buffer_as.buffer_size;
    
    // lod 1,2,3,4 size
    uint32_t lod01234_as_size = 0;
    for(int lod=1; lod<=4; lod++){
        for(auto& [model_name, _] : scene_manager.models_instances){
            uint32_t blas_idx = model_name_2_lod_vi_idx[model_name][lod-1].first;
            lod01234_as_size += lod_blas[blas_idx].buffer_as.buffer_size;
        }
        lod01234_as_size += lod_tlas[lod].buffer_as.buffer_size;
    }
    return {lod0_as_size, lod01234_as_size};
}

}






