#include <vulkan/vulkan.hpp>
#define VK_VERSION_1_0
#include <dds_image/include/dds.hpp>
#include "DdsLoader.hpp"
#include "../Utility/Error.hpp"

namespace Irori::Scene{

    DDS_Vk_Data load_dds(const uint8_t* ptr, size_t filesize){
        dds::Image dds_image;
        dds::ReadResult read_result = dds::readImage(ptr, filesize, &dds_image);
        
        ASSERT_WITH_MSG(read_result == dds::ReadResult::Success, "Read dds failed.");

        auto block_size = dds::getBlockSize(dds_image.format);

        return DDS_Vk_Data{
            .format     = vk::Format(change_format_DX_to_VK_srgb_to_unorm(dds_image.format, dds_image.supportsAlpha)),
            .block_size = dds::getBlockSize(dds_image.format),
            .numMips    = dds_image.numMips,
            .arraySize  = dds_image.arraySize,
            .width      = dds_image.width,
            .height     = dds_image.height,
            .depth      = dds_image.depth,
            .dimension  = dds_image.dimension,
            .supportsAlpha = dds_image.supportsAlpha,
            .mipmaps    = dds_image.mipmaps
        };
    }

    bool check_dds_header(uint8_t* ptr){
        // Read the magic number
        const auto* ddsMagic = reinterpret_cast<const uint32_t*>(ptr);

        // Validate header. A DWORD (magic number) containing the four character code value 'DDS '
        // (0x20534444).
        if (*ddsMagic == dds::DdsMagicNumber::DDS) return true;
        else return false;
    }

    VkFormat change_format_DX_to_VK_srgb_to_unorm(DXGI_FORMAT format, bool alphaFlag){
        switch (format) {
            case DXGI_FORMAT_BC1_UNORM: {
                if (alphaFlag)
                    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                else
                    return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            }
            case DXGI_FORMAT_BC1_UNORM_SRGB: {
                if (alphaFlag)
                    return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
                else
                    return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
            }

            case DXGI_FORMAT_BC2_UNORM:
                return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC2_UNORM_SRGB:
                return VK_FORMAT_BC2_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC3_UNORM_SRGB:
                return VK_FORMAT_BC3_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_UNORM:
                return VK_FORMAT_BC4_UNORM_BLOCK;
            case DXGI_FORMAT_BC4_SNORM:
                return VK_FORMAT_BC4_SNORM_BLOCK;
            case DXGI_FORMAT_BC5_UNORM:
                return VK_FORMAT_BC5_UNORM_BLOCK;
            case DXGI_FORMAT_BC5_SNORM:
                return VK_FORMAT_BC5_SNORM_BLOCK;
			case DXGI_FORMAT_BC7_UNORM:
				return VK_FORMAT_BC7_UNORM_BLOCK;
			case DXGI_FORMAT_BC7_UNORM_SRGB:
				return VK_FORMAT_BC7_UNORM_BLOCK;

			// 8-bit wide formats
			case DXGI_FORMAT_R8_UNORM:
                return VK_FORMAT_R8_UNORM;
			case DXGI_FORMAT_R8_UINT:
                return VK_FORMAT_R8_UINT;
			case DXGI_FORMAT_R8_SNORM:
                return VK_FORMAT_R8_SNORM;
			case DXGI_FORMAT_R8_SINT:
                return VK_FORMAT_R8_SINT;
#if defined(VK_KHR_maintenance5)
			case DXGI_FORMAT_A8_UNORM:
                return VK_FORMAT_A8_UNORM_KHR;
#endif

			// 16-bit wide formats
            case DXGI_FORMAT_R8G8_UNORM:
                return VK_FORMAT_R8G8_UNORM;
            case DXGI_FORMAT_R8G8_UINT:
                return VK_FORMAT_R8G8_UINT;
            case DXGI_FORMAT_R8G8_SNORM:
                return VK_FORMAT_R8G8_SNORM;
            case DXGI_FORMAT_R8G8_SINT:
                return VK_FORMAT_R8G8_SINT;

            case DXGI_FORMAT_R16_FLOAT:
                return VK_FORMAT_R16_SFLOAT;
            case DXGI_FORMAT_R16_UNORM:
                return VK_FORMAT_R16_UNORM;
            case DXGI_FORMAT_R16_UINT:
                return VK_FORMAT_R16_UINT;
            case DXGI_FORMAT_R16_SNORM:
                return VK_FORMAT_R16_SNORM;
            case DXGI_FORMAT_R16_SINT:
                return VK_FORMAT_R16_SINT;

			case DXGI_FORMAT_B5G5R5A1_UNORM:
				return VK_FORMAT_B5G5R5A1_UNORM_PACK16;
			case DXGI_FORMAT_B5G6R5_UNORM:
				return VK_FORMAT_B5G6R5_UNORM_PACK16;
			case DXGI_FORMAT_B4G4R4A4_UNORM:
				return VK_FORMAT_B4G4R4A4_UNORM_PACK16;

			// 32-bit wide formats
            case DXGI_FORMAT_R8G8B8A8_UNORM:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UNORM_SRGB:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case DXGI_FORMAT_R8G8B8A8_UINT:
                return VK_FORMAT_R8G8B8A8_UINT;
            case DXGI_FORMAT_R8G8B8A8_SNORM:
                return VK_FORMAT_R8G8B8A8_SNORM;
            case DXGI_FORMAT_R8G8B8A8_SINT:
                return VK_FORMAT_R8G8B8A8_SINT;
            case DXGI_FORMAT_B8G8R8A8_UNORM:
                return VK_FORMAT_B8G8R8A8_UNORM;
            case DXGI_FORMAT_B8G8R8A8_UNORM_SRGB:
                return VK_FORMAT_B8G8R8A8_UNORM;

            case DXGI_FORMAT_R16G16_FLOAT:
                return VK_FORMAT_R16G16_SFLOAT;
			case DXGI_FORMAT_R16G16_UNORM:
				return VK_FORMAT_R16G16_UNORM;
			case DXGI_FORMAT_R16G16_UINT:
				return VK_FORMAT_R16G16_UINT;
			case DXGI_FORMAT_R16G16_SNORM:
				return VK_FORMAT_R16G16_SNORM;
			case DXGI_FORMAT_R16G16_SINT:
				return VK_FORMAT_R16G16_SINT;

			case DXGI_FORMAT_R32_FLOAT:
				return VK_FORMAT_R32_SFLOAT;
			case DXGI_FORMAT_R32_UINT:
				return VK_FORMAT_R32_UINT;
			case DXGI_FORMAT_R32_SINT:
				return VK_FORMAT_R32_SINT;

			case DXGI_FORMAT_R9G9B9E5_SHAREDEXP:
				return VK_FORMAT_E5B9G9R9_UFLOAT_PACK32;
			case DXGI_FORMAT_R10G10B10A2_UNORM:
				return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
			case DXGI_FORMAT_R10G10B10A2_UINT:
				return VK_FORMAT_A2B10G10R10_UINT_PACK32;
			case DXGI_FORMAT_R11G11B10_FLOAT:
				return VK_FORMAT_B10G11R11_UFLOAT_PACK32;

			// 64-bit wide formats
            case DXGI_FORMAT_R16G16B16A16_FLOAT:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
            case DXGI_FORMAT_R16G16B16A16_SINT:
                return VK_FORMAT_R16G16B16A16_SINT;
            case DXGI_FORMAT_R16G16B16A16_UINT:
                return VK_FORMAT_R16G16B16A16_UINT;
            case DXGI_FORMAT_R16G16B16A16_UNORM:
                return VK_FORMAT_R16G16B16A16_UNORM;
            case DXGI_FORMAT_R16G16B16A16_SNORM:
                return VK_FORMAT_R16G16B16A16_SNORM;

            case DXGI_FORMAT_R32G32_FLOAT:
                return VK_FORMAT_R32G32_SFLOAT;
			case DXGI_FORMAT_R32G32_UINT:
				return VK_FORMAT_R32G32_UINT;
			case DXGI_FORMAT_R32G32_SINT:
				return VK_FORMAT_R32G32_SINT;

			// 96-bit wide formats
			case DXGI_FORMAT_R32G32B32_FLOAT:
				return VK_FORMAT_R32G32B32_SFLOAT;
			case DXGI_FORMAT_R32G32B32_UINT:
				return VK_FORMAT_R32G32B32_UINT;
			case DXGI_FORMAT_R32G32B32_SINT:
				return VK_FORMAT_R32G32B32_SINT;

			// 128-bit wide formats
			case DXGI_FORMAT_R32G32B32A32_FLOAT:
				return VK_FORMAT_R32G32B32A32_SFLOAT;
			case DXGI_FORMAT_R32G32B32A32_UINT:
				return VK_FORMAT_R32G32B32A32_UINT;
			case DXGI_FORMAT_R32G32B32A32_SINT:
				return VK_FORMAT_R32G32B32A32_SINT;

            case DXGI_FORMAT_R8G8_B8G8_UNORM:
            case DXGI_FORMAT_G8R8_G8B8_UNORM:
            case DXGI_FORMAT_YUY2:
            default:
                return VK_FORMAT_UNDEFINED;
        }

    }
    
}