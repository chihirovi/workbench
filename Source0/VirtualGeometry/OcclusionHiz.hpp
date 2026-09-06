#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueOcclusionHizPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueOcclusionHizPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueOcclusionHizPipeline(BOOST_RV_REF(UniqueOcclusionHizPipeline) rhs) = default;               //move constractor
    UniqueOcclusionHizPipeline& operator=(BOOST_RV_REF(UniqueOcclusionHizPipeline) rhs) = default;    //move assignment
    
    UniqueOcclusionHizPipeline(){;}
    UniqueOcclusionHizPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, int w, int h, Irori::Core::UniqueDescriptorSets& global_descsets);
};

}