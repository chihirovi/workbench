#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueClusterSelectPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueClusterSelectPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueClusterSelectPipeline(BOOST_RV_REF(UniqueClusterSelectPipeline) rhs) = default;               //move constractor
    UniqueClusterSelectPipeline& operator=(BOOST_RV_REF(UniqueClusterSelectPipeline) rhs) = default;    //move assignment
    
    UniqueClusterSelectPipeline(){;}
    UniqueClusterSelectPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, uint32_t cluster_sum, Irori::Core::UniqueDescriptorSets& global_descsets);

};

}