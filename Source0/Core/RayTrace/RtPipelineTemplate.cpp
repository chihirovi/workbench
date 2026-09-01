#pragma once
#include "RtPipelineTemplate.hpp"

namespace Irori::Core{

UniqueRtPipelineTemplate::UniqueRtPipelineTemplate(
    VulkanContext* vc, 
    ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource_info, 
    ShaderReflect::DescsetsInfo& global_descsets_info){
    
    vulkan_context = vc;
    
    uint32_t shader_num = pipeline_shader_resource_info.shader_infos.size();
    ASSERT_WITH_MSG((shader_num == 3 || shader_num == 4), std::format("Irori only support, 1 raygen group, 1 hit group, 1 miss group. shader count must be 3 or 4, here shader count is [{}].", shader_num));
    
    int raygen_idx = -1, chit_idx = -1, miss_idx = -1, any_idx = -1;
    for(auto [idx, shader_info] : pipeline_shader_resource_info.shader_infos|std::views::enumerate){
        if(shader_info.stage_flag == vk::ShaderStageFlagBits::eRaygenKHR)       raygen_idx = idx;
        if(shader_info.stage_flag == vk::ShaderStageFlagBits::eClosestHitKHR)   chit_idx = idx;        
        if(shader_info.stage_flag == vk::ShaderStageFlagBits::eMissKHR)         miss_idx = idx;        
        if(shader_info.stage_flag == vk::ShaderStageFlagBits::eAnyHitKHR)       any_idx= idx;        
    }
    
    ASSERT_WITH_MSG((raygen_idx >= 0), "RT pipeline must have 1 raygen shader.");
    ASSERT_WITH_MSG((miss_idx >= 0), "RT pipeline must have 1 miss shader.");
    ASSERT_WITH_MSG((chit_idx >= 0), "RT pipeline must have 1 chit shader.");
    
    
    // create desc set
    descriptor_sets = UniqueDescriptorSets::Local(vulkan_context, pipeline_shader_resource_info.local_descsets_info, global_descsets_info);
    
    // create shader modules
    shader_modules =  Core::UniqueShaderModules(vulkan_context, pipeline_shader_resource_info.shader_infos);

    // create shader group
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shader_groups_info = {
        //raygen group (group handle index = 0) 
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(raygen_idx),//shader module idx

        //miss group (group handle index = 1)
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eGeneral)
            .setGeneralShader(miss_idx),//shader module idx

        //hit group (group handle index = 2)
        vk::RayTracingShaderGroupCreateInfoKHR()
            .setType(vk::RayTracingShaderGroupTypeKHR::eTrianglesHitGroup)
            .setClosestHitShader(chit_idx),//shader module idx
    };

    if(any_idx >= 0){
        shader_groups_info.back().setAnyHitShader(any_idx);   
    }

    // create pipeline
    auto temp = descriptor_sets.get_descset_layouts();
    pipeline_layout = vulkan_context->device->createPipelineLayoutUnique(
        vk::PipelineLayoutCreateInfo()
            .setSetLayouts(temp)
    );
    
    auto pipeline_create_info = vk::RayTracingPipelineCreateInfoKHR()
        .setLayout(*pipeline_layout)
        .setStages(shader_modules.pipeline_shader_create_infos)
        .setGroups(shader_groups_info)
        .setMaxPipelineRayRecursionDepth(1);
    
    auto result = vulkan_context->device->createRayTracingPipelineKHRUnique(
        nullptr,
        nullptr,
        pipeline_create_info
    );
    ASSERT_WITH_MSG((result.result == vk::Result::eSuccess), "create RT pipeline failed.");
    pipeline = std::move(result.value);
    
    // create sbt
    sbt = std::move(RayTrace::UniqueSBT(vulkan_context, *pipeline, shader_groups_info));
   
    RayTrace::SbtInfo rgen_sbt_info{  
        RayTrace::SBT_TYPE::RGEN,
        {0}, //shader rgen group handle idx
    };

    RayTrace::SbtInfo miss_sbt_info{  
        RayTrace::SBT_TYPE::MISS,
        {1}, //shader miss group handle idx
    };

    RayTrace::SbtInfo hit_sbt_info{   
        RayTrace::SBT_TYPE::HIT,
        {2}, //shader hit group handle idx 
    };

    sbt .add_sbt(rgen_sbt_info)
        .add_sbt(miss_sbt_info)
        .add_sbt(hit_sbt_info); 
}

UniqueRtPipelineTemplate& UniqueRtPipelineTemplate::bind_pipeline(vk::CommandBuffer& cmd_buf){
    cmd_buf.bindPipeline(
        vk::PipelineBindPoint::eRayTracingKHR,
        *pipeline
    );
    return *this;
}

void UniqueRtPipelineTemplate::bind_destsets(vk::CommandBuffer& cmd_buf){
    descriptor_sets.bind_local_descriptor_sets(
        cmd_buf,
        *pipeline_layout,
        vk::PipelineBindPoint::eRayTracingKHR
    );
}

}