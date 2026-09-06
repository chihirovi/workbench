#pragma once

#include "../Core/Rasterize/RasterPipelineTemplate.hpp"
#include "../Core/Common/Buffer.hpp"

namespace Irori::VirtualGeometry{

class UniqueClusterMeshPipeline{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueClusterMeshPipeline);

public:
    enum class Stage : uint32_t{
        OC_1 = 0,
        OC_2 = sizeof(uint32_t) * 8,
    };

public:
    Irori::Core::UniqueRasterPipelineTemplate rasterize_pipeline_template;

public:
    UniqueClusterMeshPipeline (BOOST_RV_REF(UniqueClusterMeshPipeline) rhs) = default;               //move constractor
    UniqueClusterMeshPipeline& operator=(BOOST_RV_REF(UniqueClusterMeshPipeline) rhs) = default;    //move assignment
    UniqueClusterMeshPipeline(){;}
    UniqueClusterMeshPipeline(
        Core::VulkanContext* vc, 
        uint32_t w, uint32_t h,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info
    );

    void cmd_dispatch(
        vk::CommandBuffer& cb, 
        Core::UniqueBuffer& indirect_buffer, 
        Stage stage,
        Irori::Core::UniqueDescriptorSets& global_descsets
    );
};



}