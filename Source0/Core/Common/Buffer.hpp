#pragma once
#include "VulkanContext.hpp"
#include "Submit.hpp"
#include <functional>

namespace Irori::Core{
    
class UniqueBuffer{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueBuffer);
    vk::DeviceAddress device_address    = 0;

public:
    VulkanContext* vulkan_context       = nullptr;
    size_t buffer_size                  = 0;
    VkBuffer buffer                     = VK_NULL_HANDLE;
    VmaAllocation vma_allocation        = nullptr;
    void* mapped_memory                 = nullptr;
    bool is_host_visible                = false;

public:
    UniqueBuffer();
    UniqueBuffer(VulkanContext* vc, size_t size, VkBufferCreateInfo buffer_create_info, VmaAllocationCreateInfo vma_alloc_create_info, bool _is_host_visible, size_t alignment);
    ~UniqueBuffer();
    UniqueBuffer(BOOST_RV_REF(UniqueBuffer) arg);               //move constractor
    UniqueBuffer& operator=(BOOST_RV_REF(UniqueBuffer) arg);    //move assignment

    void* get_mapped_addr_for_read();
    vk::Buffer get_raw_buffer();
    void set_device_address();
    vk::DeviceAddress get_device_address();

    void transfer_data(void* data, size_t size);
    //void cmd_transfer_data(void* data, size_t size);
    void transfer_data_to_device_mem(void* data, size_t size);
    void transfer_data_to_host_visible_mem(void* data, size_t size);
    void write_data_to_host_visible_mem(std::function<size_t(void*)> writer);
    void cmd_copy_to_buffer(vk::CommandBuffer& cb, UniqueBuffer& dst_buffer, size_t size);
    
    void map_memory();
    void unmap_memory();
    
    void download_data(void* dst, uint32_t size);
    
    //factory methods
    static UniqueBuffer common(VulkanContext* vc, size_t size, bool host_visible, vk::BufferUsageFlags usage, size_t alignment = 8);
    static UniqueBuffer upload(VulkanContext* vc, size_t size, vk::BufferUsageFlags addtional_usage = {});
    static UniqueBuffer download(VulkanContext* vc, size_t size, vk::BufferUsageFlags addtional_usage = {});
};

    

}











