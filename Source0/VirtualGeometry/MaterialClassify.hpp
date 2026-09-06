#pragma once

#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueMaterialClassifyPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueMaterialClassifyPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;
    
public:

    UniqueMaterialClassifyPipeline(BOOST_RV_REF(UniqueMaterialClassifyPipeline) rhs) = default;               //move constractor
    UniqueMaterialClassifyPipeline& operator=(BOOST_RV_REF(UniqueMaterialClassifyPipeline) rhs) = default;    //move assignment
    
    UniqueMaterialClassifyPipeline(){;}
    UniqueMaterialClassifyPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, uint32_t tile_num_x, uint32_t tile_num_y, Irori::Core::UniqueDescriptorSets& global_descsets);

};

}