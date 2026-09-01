#pragma once

#define VK_VERSION_1_0
#include <dds_image/include/dds.hpp>

namespace Irori::Scene{
    struct DDS_Vk_Data{
        vk::Format format;
        uint32_t block_size;
        uint32_t numMips;
        uint32_t arraySize = 1;
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        dds::ResourceDimension dimension;
        bool supportsAlpha = false;
        std::vector<dds::span<const uint8_t>> mipmaps;
    };

    DDS_Vk_Data load_dds(const uint8_t* ptr, size_t filesize);
    VkFormat change_format_DX_to_VK_srgb_to_unorm(DXGI_FORMAT format, bool alphaFlag);
    bool check_dds_header(uint8_t* ptr);
}