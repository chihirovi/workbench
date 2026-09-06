#pragma once

#include "../Core/Rasterize/RasterPipelineTemplate.hpp"
#include "../Core/Common/Image.hpp"

namespace Irori::VirtualGeometry{

class UniqueMatID2DepthPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueMatID2DepthPipeline);

public:
    Irori::Core::UniqueRasterPipelineTemplate rasterize_pipeline_template;

public:
    UniqueMatID2DepthPipeline (BOOST_RV_REF(UniqueMatID2DepthPipeline) rhs) = default;               //move constractor
    UniqueMatID2DepthPipeline& operator=(BOOST_RV_REF(UniqueMatID2DepthPipeline) rhs) = default;    //move assignment
    UniqueMatID2DepthPipeline(){;}
    UniqueMatID2DepthPipeline(
        Core::VulkanContext* vc, 
        Irori::Core::UniqueImage& depth_image,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, Irori::Core::UniqueDescriptorSets& global_descsets);
};



}