#include "SceneAsManager.hpp"


namespace Irori::Scene{

void UniqueSceneAsManager::rebuild_tlas_blas(UniqueSceneManager& scene_manager, Render::CPU_FrameRenderData& cpu_frame_render_data){
    
    // create blas_s
    blas_s.resize(0);
    //rt_render_instance_global_id_map.resize(0);
    model_2_blas_idx_RT_render_inst_base_map.clear();

    // 一つのモデルは複数の render instance (submesh) から構成される
    // 例えば，modelA = table[0..4], modelB table = [4..5], ...
    // みたいな感じ，
    // scene_manager.model_name_to_render_inst_table は，tableのどこからどこまでがそのモデルかを記録する（上の情報そのまま）
    // ASに関しては，このモデルごとに，BLAS があり，
    // BLAS のサブメッシュの並び順は table 内の サブメッシュの並びと一致している
    // TLAS では，BLAS Instance ごとに，その BLAS の代表する model の render instance table の始まりを記録する
    // アクセスするときは，blas sub mesh の geometry index + custom index（render instance の始まり） で計算する． 
    // なので，飛ばしたりすると，ダメ，本当は変換テーブルがいるかも
    for(auto& [model_name, render_instance_range] : scene_manager.model_name_to_render_inst_table){

        uint32_t render_instance_id_base = render_instance_range.first;
        uint32_t render_instance_count = render_instance_range.second;
        
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

            auto& material = cpu_frame_render_data.material_table[render_instance.material_id_base + render_instance.material_id];

            if(material.blend_mode == Render::MaterialBlendMode::opaque){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

            }else if(material.blend_mode == Render::MaterialBlendMode::mask){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;

            }else if(material.blend_mode == Render::MaterialBlendMode::blend){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;

            }else{
                ASSERT_WITH_MSG(false, "unknown material blend mode");          
            }

            // 透明マテリアルはとりあえず無視で
            // 飛ばしたりすると，ダメ，本当は変換テーブルがいるかも
            if(material.is_transmission == 1){
                blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eNoDuplicateAnyHitInvocation;
                //blas_sub_mesh_info.geometry_frag = vk::GeometryFlagBitsKHR::eOpaque;
            }
            
            //rt_render_instance_global_id_map.push_back(render_instance_global_id);
            blas_create_info.push_back(blas_sub_mesh_info);
        }
        
        // create blas
        blas_s.push_back(std::move(RayTrace::create_blas(vulkan_context, blas_create_info)));

        // register
        model_2_blas_idx_RT_render_inst_base_map[model_name] = {
            blas_s.size()-1, 
            //rt_render_inst_global_id_map_base
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
    
    tlas = std::move(RayTrace::create_tlas(vulkan_context, tlas_create_info, blas_s));
    
    // tlal instances data buffer
    tlas_instances_data_buffer = Core::UniqueBuffer::common(
        vulkan_context,
        tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR),
        false,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
        vk::BufferUsageFlagBits::eShaderDeviceAddress|
        vk::BufferUsageFlagBits::eTransferDst,
        8
    );   

    tlas_instances_data_staging_buffer = Core::UniqueBuffer::upload(
        vulkan_context,
        tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR)
    );  
}


void UniqueSceneAsManager::prepare_tlas_update_data(UniqueSceneManager& scene_manager){
    
    // prepare update data
    std::vector<vk::AccelerationStructureInstanceKHR> accel_instances;
    accel_instances.reserve(tlas_instance_count);

    for(auto& [model_name, model_instances] : scene_manager.models_instances){

        ASSERT_WITH_MSG((model_2_blas_idx_RT_render_inst_base_map.contains(model_name)), std::format("model 2 blas_idx render inst base dosen't containt model name [{}].", model_name));
        
        auto& tmp = model_2_blas_idx_RT_render_inst_base_map[model_name];
        auto blas_idx = tmp.first;
        auto rt_render_inst_base = tmp.second;

        for(auto& [modol_inst_name, model_inst] : model_instances){

            vk::TransformMatrixKHR transform = RayTrace::convert_transform(model_inst.transform); 

            auto accel_instance = vk::AccelerationStructureInstanceKHR()
                .setTransform(transform)
                .setInstanceCustomIndex(rt_render_inst_base)
                .setMask(0xFFFFFFFF)
                .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
                .setAccelerationStructureReference(blas_s[blas_idx].get_AS_address())
                .setInstanceShaderBindingTableRecordOffset(0);

            accel_instances.push_back(accel_instance);//ここの順番は，作成済みのtlasと揃えないとダメ
        }
    }
    
    // transfer to staging
    ASSERT_WITH_MSG((tlas_instances_data_staging_buffer.buffer != VK_NULL_HANDLE), "tlas instances data staging buffer is null handle.");
    
    tlas_instances_data_staging_buffer.transfer_data(accel_instances.data(), accel_instances.size()*sizeof(vk::AccelerationStructureInstanceKHR));
}

void UniqueSceneAsManager::cmd_update_tlas_with_barrier(vk::CommandBuffer& cmd_buf){
    // copy data
    tlas_instances_data_staging_buffer.cmd_copy_to_buffer(
        cmd_buf,
        tlas_instances_data_buffer,
        tlas_instance_count*sizeof(vk::AccelerationStructureInstanceKHR)
    );
    
    // barrier
    Core::CmdBufferBarrier::transfer_dst_2_as_update_read(cmd_buf, tlas_instances_data_buffer);
    
    // update
    RayTrace::cmd_update_tlas(cmd_buf, tlas, tlas_instance_count, tlas_instances_data_buffer);
    
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