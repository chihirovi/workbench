#pragma once
#include "VulkanContext.hpp"
#include "Submit.hpp"
#include "Buffer.hpp"
#include "../../Scene/DdsLoader.hpp"

#define VK_VERSION_1_0
#include <dds_image/include/dds.hpp>

namespace Irori::Core{
    
class UniqueImage{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueImage);

public:
    VulkanContext* vulkan_context               = nullptr;
    VkImage image                               = VK_NULL_HANDLE;
    vk::UniqueImageView image_view;
    VmaAllocation vma_allocation                = nullptr;
    vk::ImageLayout current_mip0_array0_layout  = vk::ImageLayout::eUndefined;
    vk::ImageCreateInfo image_create_info;
    vk::ImageViewCreateInfo image_view_create_info;

public:
    UniqueImage();
    UniqueImage(VulkanContext* vc);
    ~UniqueImage();
    UniqueImage(BOOST_RV_REF(UniqueImage) rhs);               //move constractor
    UniqueImage& operator=(BOOST_RV_REF(UniqueImage) rhs);    //move assignment
                                                              
    void create_image(vk::ImageCreateInfo _iamge_create_info, VmaAllocationCreateInfo _vma_alloc_create_info);
    void create_image_view(vk::ImageViewCreateInfo _image_view_create_info);
    vk::Image get_raw_image(){return image;}
    void set_current_mip0_array0_layout(vk::ImageLayout layout){current_mip0_array0_layout = layout;}
    void transter_data_to_mip0_array0(void* data);
    void transter_data_to_mipN_array0(Core::UniqueBuffer& staging_buffer, uint32_t mip_level);
    void create_mipmap_from_mip0();
    void cmd_copy_to_image(vk::CommandBuffer& cb, UniqueImage& dst_image);

public: //builder methods
    struct Builder2D{
    public:
        VulkanContext* vulkan_context           = nullptr;
        uint32_t x                              = 0;
        uint32_t y                              = 0;
        vk::Format format                       = vk::Format::eUndefined;
        vk::ImageUsageFlags usages              = {};
        vk::ImageLayout init_layout             = vk::ImageLayout::eUndefined;
        uint32_t array_size                     = 1;
        vk::SampleCountFlagBits sample_count    = vk::SampleCountFlagBits::e1;
        bool is_enable_mipmap                   = false;
        void* data                              = nullptr;
        vk::Image raw_image                     = VK_NULL_HANDLE;
        int max_mip_levels = 1;

    public:
        Builder2D(VulkanContext* vc){vulkan_context = vc;};
        Builder2D& set_size_format_usage_layout(uint32_t _x, uint32_t _y, vk::Format _format, vk::ImageUsageFlags _usages, vk::ImageLayout _layout){
            x = _x, y = _y;
            format = _format;
            usages = _usages;
            init_layout = _layout;
            return *this;
        };
        Builder2D& set_2d_array_size(uint32_t _array_size){array_size = _array_size; return *this;};
        Builder2D& set_sample_count(vk::SampleCountFlagBits _sample_count){sample_count = _sample_count; return *this;};
        Builder2D& enable_mipmap(){is_enable_mipmap = true; return *this;};
        Builder2D& set_data(void* _data){data = _data; return *this;};
        Builder2D& set_raw_image(vk::Image _image){raw_image = _image; return *this;};
        Builder2D& set_max_mip_level(int level){max_mip_levels = level; return *this;}
        UniqueImage build();
    };

public: //builder methods
    struct Builder2D_DDS{
    public:
        VulkanContext* vulkan_context           = nullptr;
        Scene::DDS_Vk_Data* dds_vk_data         = nullptr;
        vk::ImageUsageFlags usages              = {};
        vk::ImageLayout init_layout             = vk::ImageLayout::eUndefined;
        
    public:
        Builder2D_DDS(VulkanContext* vc){vulkan_context = vc;};
        Builder2D_DDS& set_usage_layout(vk::ImageUsageFlags _usages, vk::ImageLayout _layout){
            usages = _usages;
            init_layout = _layout;
            return *this;
        };
        Builder2D_DDS& set_dds_vk_data(Scene::DDS_Vk_Data& _dds_vk_data){dds_vk_data = &_dds_vk_data; return *this;};
        UniqueImage build();
    };
};

//utility
vk::ImageAspectFlagBits image_view_aspect_from_format(vk::Format _format);
uint32_t bytes_from_format(vk::Format _format);

}