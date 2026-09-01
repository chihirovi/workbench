#include "Sampler.hpp"

namespace Irori::Core{
    
// Irori::core::unique sampler
UniqueSampler::UniqueSampler(
    VulkanContext* vc, 
    vk::Filter mag_filter, 
    vk::Filter min_filter, 
    std::optional<float> anisotropy, 
    vk::SamplerMipmapMode mip_mode, 
    std::pair<float, float> min_max_mip_lod){
    
    vk::SamplerCreateInfo sampler_create_info;

    if(anisotropy){
        sampler_create_info
            .setAnisotropyEnable(vk::True)   
            .setMaxAnisotropy(anisotropy.value());
    }else{
        
        sampler_create_info
            .setAnisotropyEnable(vk::False);   
    }

    sampler_create_info
        .setMagFilter(mag_filter)
        .setMinFilter(min_filter)
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setAddressModeW(vk::SamplerAddressMode::eRepeat)
        .setBorderColor(vk::BorderColor::eIntOpaqueBlack)
        .setUnnormalizedCoordinates(vk::False)
        .setCompareEnable(vk::False)
        .setCompareOp(vk::CompareOp::eAlways)
        .setMipmapMode(mip_mode)  //trilinear
        .setMipLodBias(0.0)
        .setMinLod(min_max_mip_lod.first)
        .setMaxLod(min_max_mip_lod.second);
    
    sampler = vc->device->createSamplerUnique(sampler_create_info);
}

// Irori::core::uniaue sampler pool
UniqueSamplerPool::UniqueSamplerPool(VulkanContext* vc){
    vulkan_context = vc;   
    
    samplers.resize(13);

    // 0
    samplers[TYPE::MAG_NEAREST_MIN_NEAREST_MIP_NONE] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, 0.0f}                        //min lod level
    ));

    // 1
    samplers[TYPE::MAG_NEAREST_MIN_LINEAR_MIP_NONE] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, 0.0f}                        //min lod level
    ));

    // 2
    samplers[TYPE::MAG_NEAREST_MIN_NEAREST_MIP_NEAREST] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 3
    samplers[TYPE::MAG_NEAREST_MIN_LINEAR_MIP_NEAREST] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 4
    samplers[TYPE::MAG_NEAREST_MIN_NEAREST_MIP_LINEAR] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eLinear,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 5
    samplers[TYPE::MAG_NEAREST_MIN_LINEAR_MIP_LINEAR] = std::move(UniqueSampler(
        vc,
        vk::Filter::eNearest,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eLinear,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 6
    samplers[TYPE::MAG_LINEAR_MIN_NEAREST_MIP_NONE] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, 0.0f}                        //min lod level
    ));

    // 7
    samplers[TYPE::MAG_LINEAR_MIN_LINEAR_MIP_NONE] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, 0.0f}                        //min lod level
    ));

    // 8
    samplers[TYPE::MAG_LINEAR_MIN_NEAREST_MIP_NEAREST] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 9
    samplers[TYPE::MAG_LINEAR_MIN_LINEAR_MIP_NEAREST] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 10
    samplers[TYPE::MAG_LINEAR_MIN_NEAREST_MIP_LINEAR] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eNearest,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eLinear,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 11
    samplers[TYPE::MAG_LINEAR_MIN_LINEAR_MIP_LINEAR] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eLinear,               //min
        16.0f,                              //anistropy
        vk::SamplerMipmapMode::eLinear,    //mip mode
        {0.0f, vk::LodClampNone}            //min lod level
    ));

    // 12
    samplers[TYPE::DDGI_SAMPLER] = std::move(UniqueSampler(
        vc,
        vk::Filter::eLinear,               //mag
        vk::Filter::eLinear,               //min
        std::nullopt,                              //anistropy
        vk::SamplerMipmapMode::eNearest,    //mip mode
        {0.0f, 0.0f}            //min lod level
    ));
}

std::vector<vk::Sampler> UniqueSamplerPool::get_samplers(){
    std::vector<vk::Sampler> vk_samplers;
    for(auto& sampler : samplers){
        vk_samplers.push_back(sampler.sampler.get());
    }
    return vk_samplers;
}

}
















