#pragma once
#include "VulkanContext.hpp"

namespace Irori::Core{

class UniqueSampler{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSampler);

public:
    vk::UniqueSampler sampler;

public:
    UniqueSampler(){;}
    UniqueSampler(VulkanContext* vc, vk::Filter mag_filter, vk::Filter min_filter, std::optional<float> anisotropy, vk::SamplerMipmapMode mip_mode, std::pair<float, float> min_max_mip_lod);
    UniqueSampler(BOOST_RV_REF(UniqueSampler) rhs) = default;               //move constractor
    UniqueSampler& operator=(BOOST_RV_REF(UniqueSampler) rhs) = default;    //move assignment
    
};
    
class UniqueSamplerPool{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSamplerPool);

public:
    enum TYPE: uint32_t{
        MAG_NEAREST_MIN_NEAREST_MIP_NONE    = 0,  //mipmapを使わない
        MAG_NEAREST_MIN_LINEAR_MIP_NONE     = 1,   //mipmapを使わない 
        MAG_NEAREST_MIN_NEAREST_MIP_NEAREST = 2,
        MAG_NEAREST_MIN_LINEAR_MIP_NEAREST  = 3,
        MAG_NEAREST_MIN_NEAREST_MIP_LINEAR  = 4,
        MAG_NEAREST_MIN_LINEAR_MIP_LINEAR   = 5,
        MAG_LINEAR_MIN_NEAREST_MIP_NONE     = 6,   //mipmapを使わない
        MAG_LINEAR_MIN_LINEAR_MIP_NONE      = 7,  //mipmapを使わない
        MAG_LINEAR_MIN_NEAREST_MIP_NEAREST  = 8,   
        MAG_LINEAR_MIN_LINEAR_MIP_NEAREST   = 9,
        MAG_LINEAR_MIN_NEAREST_MIP_LINEAR   = 10,
        MAG_LINEAR_MIN_LINEAR_MIP_LINEAR    = 11,
        DDGI_SAMPLER                        = 12,
    };
    
public:
    VulkanContext* vulkan_context = nullptr;
    std::vector<Irori::Core::UniqueSampler> samplers;

public:
    UniqueSamplerPool(){;}
    UniqueSamplerPool(VulkanContext* vc);
    UniqueSamplerPool(BOOST_RV_REF(UniqueSamplerPool) rhs) = default;               //move constractor
    UniqueSamplerPool& operator=(BOOST_RV_REF(UniqueSamplerPool) rhs) = default;    //move assignment
    std::vector<vk::Sampler> get_samplers();
};


}