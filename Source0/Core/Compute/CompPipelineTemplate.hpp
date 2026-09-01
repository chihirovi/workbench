#pragma once
#include "../Common/VulkanContext.hpp"
#include "../Common/ShaderModule.hpp"
#include "../Common/DescriptorSets.hpp"

namespace Irori::Core{
    
class UniqueCompPipelineTemplate{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueCompPipelineTemplate);
    
public:
    VulkanContext* vulkan_context = nullptr;
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    UniqueShaderModules shader_modules;
    UniqueDescriptorSets descriptor_sets;

public:
    UniqueCompPipelineTemplate(){;}
    UniqueCompPipelineTemplate(VulkanContext* vc, ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource_info, ShaderReflect::DescsetsInfo& global_descsets_info);
    virtual ~UniqueCompPipelineTemplate(){;}
    UniqueCompPipelineTemplate(BOOST_RV_REF(UniqueCompPipelineTemplate) rhs) = default;               //move constractor
    UniqueCompPipelineTemplate& operator=(BOOST_RV_REF(UniqueCompPipelineTemplate) rhs) = default;    //move assignment

    UniqueCompPipelineTemplate& bind_pipeline(vk::CommandBuffer& cb);
    void bind_local_descsets(vk::CommandBuffer& cb);
}; 




}
