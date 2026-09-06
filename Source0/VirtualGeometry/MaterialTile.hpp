#pragma once

#include "../Core/Rasterize/RasterPipelineTemplate.hpp"
#include "../Core/Common/Image.hpp"
#include "../Core/Common/Buffer.hpp"

namespace Irori::VirtualGeometry{

class UniqueMaterialTilePipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueMaterialTilePipeline);

public:
    Irori::Core::UniqueRasterPipelineTemplate rasterize_pipeline_template;

public:
    UniqueMaterialTilePipeline (BOOST_RV_REF(UniqueMaterialTilePipeline) rhs) = default;               //move constractor
    UniqueMaterialTilePipeline& operator=(BOOST_RV_REF(UniqueMaterialTilePipeline) rhs) = default;    //move assignment
    UniqueMaterialTilePipeline(){;}
    UniqueMaterialTilePipeline(
        Core::VulkanContext* vc, 
        Irori::Core::UniqueImage& depth_image,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(vk::CommandBuffer& cb, Core::UniqueBuffer& arg_buffer, Irori::Core::UniqueDescriptorSets& global_descsets);
};



}