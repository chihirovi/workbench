#pragma once

#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueClearVGPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueClearVGPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueClearVGPipeline(BOOST_RV_REF(UniqueClearVGPipeline) rhs) = default;               //move constractor
    UniqueClearVGPipeline& operator=(BOOST_RV_REF(UniqueClearVGPipeline) rhs) = default;    //move assignment
    
    UniqueClearVGPipeline(){;}
    UniqueClearVGPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, uint32_t w, uint32_t h, Irori::Core::UniqueDescriptorSets& global_descsets);

};

}