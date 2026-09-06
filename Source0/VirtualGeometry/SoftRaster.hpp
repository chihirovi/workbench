#pragma once
#include "../Core/Compute/CompPipelineTemplate.hpp"

namespace Irori::VirtualGeometry{

class UniqueSoftRasterPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSoftRasterPipeline);

public:
    Core::UniqueCompPipelineTemplate comp_pipeline_templtate;

public:
    enum class Stage : uint32_t{
        OC_1 = sizeof(uint32_t) * 4,
        OC_2 = sizeof(uint32_t) * 12,
    };
    
public:

    UniqueSoftRasterPipeline(BOOST_RV_REF(UniqueSoftRasterPipeline) rhs) = default;               //move constractor
    UniqueSoftRasterPipeline& operator=(BOOST_RV_REF(UniqueSoftRasterPipeline) rhs) = default;    //move assignment
    
    UniqueSoftRasterPipeline(){;}
    UniqueSoftRasterPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(
        vk::CommandBuffer& cb, 
        Core::UniqueBuffer& arg_buffer, 
        Stage stage,
        Irori::Core::UniqueDescriptorSets& global_descsets
    );
};

}