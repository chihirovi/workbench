#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueClusterSelectPipeline2{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueClusterSelectPipeline2);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueClusterSelectPipeline2(BOOST_RV_REF(UniqueClusterSelectPipeline2) rhs) = default;               //move constractor
    UniqueClusterSelectPipeline2& operator=(BOOST_RV_REF(UniqueClusterSelectPipeline2) rhs) = default;    //move assignment
    
    UniqueClusterSelectPipeline2(){;}
    UniqueClusterSelectPipeline2(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, Core::UniqueBuffer& arg_buffer, Irori::Core::UniqueDescriptorSets& global_descsets);
};

}