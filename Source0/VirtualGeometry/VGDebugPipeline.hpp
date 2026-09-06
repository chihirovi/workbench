#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueVGDebugPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueVGDebugPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueVGDebugPipeline(BOOST_RV_REF(UniqueVGDebugPipeline) rhs) = default;               //move constractor
    UniqueVGDebugPipeline& operator=(BOOST_RV_REF(UniqueVGDebugPipeline) rhs) = default;    //move assignment
    
    UniqueVGDebugPipeline(){;}
    UniqueVGDebugPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, uint32_t w, uint32_t h, Irori::Core::UniqueDescriptorSets& global_descsets);

};

}