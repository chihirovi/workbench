#include "SBT.hpp"

namespace Irori::RayTrace{
    
UniqueSBT::UniqueSBT(
    Core::VulkanContext* vc, 
    vk::Pipeline& rt_pipeline, 
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shader_groups_info){

    vulkan_context = vc;

    set_sbt_props();

    get_all_handles(shader_groups_info, rt_pipeline);
}


void UniqueSBT::set_sbt_props(){
    auto rt_pipeline_properties = getRayTracingProps();
    handle_size         = rt_pipeline_properties.shaderGroupHandleSize;
    handle_alignment    = rt_pipeline_properties.shaderGroupHandleAlignment;
    base_alignment      = rt_pipeline_properties.shaderGroupBaseAlignment;      
}


void UniqueSBT::get_all_handles(
    std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shader_groups_info, 
    vk::Pipeline& rt_pipeline){

    handle_storage.resize(shader_groups_info.size()*handle_size);

    auto result = vulkan_context->device->getRayTracingShaderGroupHandlesKHR(
        rt_pipeline, 
        0,      //first group
        shader_groups_info.size(),
        handle_storage.size(), 
        handle_storage.data()
    );

    //check result
    ASSERT_WITH_MSG((result == vk::Result::eSuccess), std::format("Failed to get ray tracing shader group handles, [{}]", vk::to_string(result)));
}

UniqueSBT& UniqueSBT::add_sbt(SbtInfo& sbt_info){

    auto addr_region = vk::StridedDeviceAddressRegionKHR()
        .setStride(recode_stride(sbt_info));
    
    //location to add sbt buffer and sbt region
    std::vector<Core::UniqueBuffer>* sbt_buffers = nullptr;        
    std::vector<vk::StridedDeviceAddressRegionKHR>* addr_regions = nullptr;
    
    //set dst 
    switch (sbt_info.sbt_type){

        case SBT_TYPE::RGEN:

            /*
            if(sbt_info.handle_idxes.size() != 1){
                exitStr("rgen sbt size must be 1.");
            }*/

            addr_region.setSize(addr_region.stride); //special rule
            sbt_buffers = &rgen_sbt_buffers;
            addr_regions = &rgen_addr_regions;
            break;

        case SBT_TYPE::MISS:

            addr_region.setSize(addr_region.stride*sbt_info.handle_idxes.size());
            sbt_buffers = &miss_sbt_buffers;
            addr_regions = &miss_addr_regions;
            break;

        case SBT_TYPE::HIT:

            addr_region.setSize(addr_region.stride*sbt_info.handle_idxes.size());
            sbt_buffers = &hit_sbt_buffers;
            addr_regions = &hit_addr_regions;
            break;

        default:
            break;
    }
        

    
    //create sbt buffer and sbt region
    auto sbt_buffer = create_sbt_data_buffer(addr_region, sbt_info);
    
    addr_region.setDeviceAddress(sbt_buffer.get_device_address());

    //add sbt buffer and sbt region
    sbt_buffers->push_back(std::move(sbt_buffer));
    addr_regions->push_back(addr_region);
    
    return *this;
}

/*utility*/
uint32_t UniqueSBT::align_up(uint32_t size, uint32_t alignment) {
    return (size + alignment - 1) & ~(alignment - 1); 
}

uint32_t UniqueSBT::recode_stride(SbtInfo& sbt_info){
    return align_up(
        handle_size+sbt_info.additional_data_byte,
        handle_alignment
    );
}

Core::UniqueBuffer UniqueSBT::create_sbt_data_buffer(
    vk::StridedDeviceAddressRegionKHR& region,
    SbtInfo& sbt_info){

    std::vector<uint8_t> tmp_recode_buf(region.size);

    //copy handles
    for(int i=0; i<sbt_info.handle_idxes.size(); i++){
        std::memcpy(
            tmp_recode_buf.data()+i*region.stride,  //dst
            handle_storage.data()+sbt_info.handle_idxes[i]*handle_size,  //src
            handle_size //size
        );    
    }
    
    auto buffer = Core::UniqueBuffer::common(
        vulkan_context,
        region.size,
        false,
        vk::BufferUsageFlagBits::eShaderBindingTableKHR|
        vk::BufferUsageFlagBits::eTransferDst|
        vk::BufferUsageFlagBits::eShaderDeviceAddress,
        base_alignment
    );
    buffer.transfer_data(tmp_recode_buf.data(), region.size);
    return std::move(buffer);
}

vk::PhysicalDeviceRayTracingPipelinePropertiesKHR UniqueSBT::getRayTracingProps() {
    auto device_properties = vulkan_context->physical_device
        .getProperties2<vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();

    return device_properties.get<vk::PhysicalDeviceRayTracingPipelinePropertiesKHR>();
}


}