#pragma once
#include "../Common/VulkanContext.hpp"
#include "../Common/ShaderModule.hpp"
#include "../Common/DescriptorSets.hpp"
#include "SBT.hpp"
#include <ranges>


namespace Irori::Core{
    
class UniqueRtPipelineTemplate{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueRtPipelineTemplate);

public:
    VulkanContext*              vulkan_context = nullptr;
    vk::UniquePipeline          pipeline;
    vk::UniquePipelineLayout    pipeline_layout;
    UniqueShaderModules         shader_modules;
    UniqueDescriptorSets        descriptor_sets;
    RayTrace::UniqueSBT         sbt;

public:
    UniqueRtPipelineTemplate(BOOST_RV_REF(UniqueRtPipelineTemplate) rhs) = default;               //move constractor
    UniqueRtPipelineTemplate& operator=(BOOST_RV_REF(UniqueRtPipelineTemplate) rhs) = default;    //move assignment

    UniqueRtPipelineTemplate(){;}
    UniqueRtPipelineTemplate(VulkanContext* vc, ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource_info, ShaderReflect::DescsetsInfo& global_descsets_info);
    
    UniqueRtPipelineTemplate& bind_pipeline(vk::CommandBuffer& cmd_buf);
    void bind_destsets(vk::CommandBuffer& cmd_buf);
    
    vk::StridedDeviceAddressRegionKHR raygen_region(){return sbt.rgen_addr_regions[0];}
    vk::StridedDeviceAddressRegionKHR hit_region(){return sbt.hit_addr_regions[0];}
    vk::StridedDeviceAddressRegionKHR miss_region(){return sbt.miss_addr_regions[0];}
};


}