#include "CompPipelineTemplate.hpp"

namespace Irori::Core{
    
UniqueCompPipelineTemplate::UniqueCompPipelineTemplate(
    VulkanContext* vc, 
    ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource_info,
    ShaderReflect::DescsetsInfo& global_descsets_info){
    
    vulkan_context = vc;
    ASSERT_WITH_MSG((pipeline_shader_resource_info.shader_infos.size() == 1), std::format("comp pipeline template : shader num must be 1"));
    ASSERT_WITH_MSG((pipeline_shader_resource_info.shader_infos[0].stage_flag == vk::ShaderStageFlagBits::eCompute), std::format("comp pipeline template : shader stage must be compute, but this is {}", vk::to_string(pipeline_shader_resource_info.shader_infos[0].stage_flag)));
    
    //create desc set
    descriptor_sets = UniqueDescriptorSets::Local(vulkan_context, pipeline_shader_resource_info.local_descsets_info, global_descsets_info);
    
    //create shader modules
    shader_modules = UniqueShaderModules(vulkan_context, pipeline_shader_resource_info.shader_infos);
    
    //create pipeline layout
    auto temp = descriptor_sets.get_descset_layouts();
    vk::PipelineLayoutCreateInfo layout_info = vk::PipelineLayoutCreateInfo()
        .setSetLayouts(temp);
    pipeline_layout = vulkan_context->device->createPipelineLayoutUnique(layout_info);

    //create pipeline
    vk::ComputePipelineCreateInfo pipeline_info = vk::ComputePipelineCreateInfo()
        .setStage(shader_modules.pipeline_shader_create_infos[0])
        .setLayout(*pipeline_layout);
    auto result = vulkan_context->device->createComputePipelineUnique(nullptr, pipeline_info);
    VKHPP_CHECK(result);
    pipeline = std::move(result.value);
}


UniqueCompPipelineTemplate& UniqueCompPipelineTemplate::bind_pipeline(vk::CommandBuffer& cb){
    cb.bindPipeline(vk::PipelineBindPoint::eCompute, *pipeline);
    return *this;
}

void UniqueCompPipelineTemplate::bind_local_descsets(vk::CommandBuffer& cb){
    descriptor_sets.bind_local_descriptor_sets(
        cb,
        *pipeline_layout,
        vk::PipelineBindPoint::eCompute
    );   
}

}