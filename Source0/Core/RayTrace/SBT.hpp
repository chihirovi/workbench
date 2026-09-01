#pragma once

#include "../Common/VulkanContext.hpp"
#include "../Common/Buffer.hpp"
#include "../Common/ShaderModule.hpp"

namespace Irori::RayTrace{

/* config */
enum struct SBT_TYPE{
    RGEN,
    MISS,
    HIT
};

struct SbtInfo{
    SBT_TYPE sbt_type;
    std::vector<uint32_t> handle_idxes;

    //---The following parameters are not planned to be used---//
    // shader record用のフィールド，基本的には使わない
    uint32_t additional_data_byte = 0; //byte count
    void*    additional_data = nullptr;//sr
};

//もともとSBTを一元管理できるようにと，vectorで作ったけど
//結局は個々のパイプラインが自分の分を自己管理するから，
//もっとシンプルなつくりでもよかったかも
//
class UniqueSBT{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSBT);

public:
    Core::VulkanContext* vulkan_context;
    

    //shader binding table buffer
    std::vector<Core::UniqueBuffer> rgen_sbt_buffers;
    std::vector<Core::UniqueBuffer> miss_sbt_buffers;
    std::vector<Core::UniqueBuffer> hit_sbt_buffers;
    
    //sbt region
    std::vector<vk::StridedDeviceAddressRegionKHR> rgen_addr_regions; 
    std::vector<vk::StridedDeviceAddressRegionKHR> miss_addr_regions;
    std::vector<vk::StridedDeviceAddressRegionKHR> hit_addr_regions;
    
    //sbt props
    uint32_t handle_size;   
    uint32_t handle_alignment;
    uint32_t base_alignment;    
    
    //handle strage
    std::vector<uint8_t> handle_storage;

public:
    UniqueSBT(BOOST_RV_REF(UniqueSBT) rhs) = default;               //move constractor
    UniqueSBT& operator=(BOOST_RV_REF(UniqueSBT) rhs) = default;    //move assignment

    UniqueSBT(){;}
    UniqueSBT(Core::VulkanContext* vc, vk::Pipeline& rt_pipeline, std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shader_groups_info);
    void get_all_handles(std::vector<vk::RayTracingShaderGroupCreateInfoKHR>& shader_groups_info, vk::Pipeline& rt_pipeline);
    UniqueSBT& add_sbt(SbtInfo& sbt_info);

    /*utilily*/
    void set_sbt_props();
    uint32_t align_up(uint32_t size, uint32_t alignment);
    uint32_t recode_stride(SbtInfo& stb_info);
    Core::UniqueBuffer create_sbt_data_buffer(vk::StridedDeviceAddressRegionKHR& region, SbtInfo& sbt_info);
    vk::PhysicalDeviceRayTracingPipelinePropertiesKHR getRayTracingProps();
};


}