#include "Buffer.hpp"

namespace Irori::Core{

UniqueBuffer::UniqueBuffer(){;}

UniqueBuffer::UniqueBuffer(
    VulkanContext* vc,
    size_t  size,
    VkBufferCreateInfo buffer_create_info,
    VmaAllocationCreateInfo vma_alloc_create_info,
    bool _is_host_visible,
    size_t alignment){
    
    vulkan_context = vc;
    buffer_size = size;
    is_host_visible = _is_host_visible;
    
    auto result = vmaCreateBufferWithAlignment(
        vulkan_context->vma_allocator,
        &buffer_create_info,
        &vma_alloc_create_info,
        alignment,
        &buffer,
        &vma_allocation,
        nullptr
    );
    
    ASSERT_WITH_MSG((result == VK_SUCCESS), "can't create buffer");
    if(is_host_visible) map_memory();
    if(buffer_create_info.usage & VkBufferUsageFlagBits::VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT){
        set_device_address();
    }
}

    
vk::Buffer UniqueBuffer::get_raw_buffer(){
    return buffer;
}

void UniqueBuffer::set_device_address(){
    device_address = vulkan_context->device->getBufferAddress(
        vk::BufferDeviceAddressInfoKHR()
            .setBuffer(buffer)
    );    
}

vk::DeviceAddress UniqueBuffer::get_device_address(){
    ASSERT_WITH_MSG((device_address != 0), "devide address is invalid value : 0");
    return device_address;
}


void UniqueBuffer::map_memory(){
   auto result = vmaMapMemory(
        vulkan_context->vma_allocator,
        vma_allocation,
        &mapped_memory
    );

    ASSERT_WITH_MSG((result == VK_SUCCESS), "can't map memory");
}

void UniqueBuffer::unmap_memory(){
    vmaUnmapMemory(
        vulkan_context->vma_allocator,
        vma_allocation
    );
}

void* UniqueBuffer::get_mapped_addr_for_read(){
    ASSERT_WITH_MSG(!(buffer==VK_NULL_HANDLE), "this Buffer is null");
    ASSERT_WITH_MSG(is_host_visible, "this buffer is not host visible");
    auto res = vmaInvalidateAllocation(
        vulkan_context->vma_allocator,
        vma_allocation,
        0,  // アロケーション内オフセット
        buffer_size     // バイト数 (VK_WHOLE_SIZE 可)
    );
    ASSERT_WITH_MSG((res == VK_SUCCESS), "vma Invalidate memory failed");
    return mapped_memory;
}

void UniqueBuffer::cmd_copy_to_buffer(vk::CommandBuffer& cb, UniqueBuffer& dst_buffer, size_t size){
    ASSERT_WITH_MSG((this->buffer != VK_NULL_HANDLE), "src buffer is vk_null_handle");
    ASSERT_WITH_MSG((dst_buffer.get_raw_buffer()!= VK_NULL_HANDLE), "dst buffer is vk_null_handle");
    if(size == 0) return;
    auto buffer_copy_info = vk::BufferCopy()
        .setSrcOffset(0)
        .setDstOffset(0)
        .setSize(size);
    cb.copyBuffer(
        this->get_raw_buffer(),
        dst_buffer.get_raw_buffer(),
        {buffer_copy_info}
    );
}

void UniqueBuffer::transfer_data(void* data, size_t size){
    ASSERT_TRUE_WITH_MSG((buffer_size < size), std::format("Cannot transfer data beyond the buffer size, buffer size [{}], data size [{}]", buffer_size, size));
    if(is_host_visible){
        transfer_data_to_host_visible_mem(data, size);
    }else{
        transfer_data_to_device_mem(data, size);
    }
}

void UniqueBuffer::transfer_data_to_device_mem(void* data, size_t size){
    auto staging_buffer = UniqueBuffer::upload(vulkan_context, size);
    staging_buffer.transfer_data(data, size);

    auto buffer_copy_info = vk::BufferCopy()
        .setSrcOffset(0)
        .setDstOffset(0)
        .setSize(size);
    Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){
        cb.copyBuffer(
            staging_buffer.get_raw_buffer(),
            this->get_raw_buffer(),
            {buffer_copy_info}
        );
    });
}

void UniqueBuffer::transfer_data_to_host_visible_mem(void* data, size_t size){
    write_data_to_host_visible_mem([&](void* map_addr)->size_t{
        std::memcpy(map_addr, data, size);
        return size;
    });
}

void UniqueBuffer::write_data_to_host_visible_mem(std::function<size_t(void*)> writer){

    ASSERT_WITH_MSG(is_host_visible, "this buffer is not host visible");

    writer(mapped_memory);//write

    VkResult res = vmaFlushAllocation( //flash
        vulkan_context->vma_allocator,
        vma_allocation,
        0,  // アロケーション内オフセット
        buffer_size     // バイト数 (VK_WHOLE_SIZE 可)
    );
    ASSERT_WITH_MSG((res == VK_SUCCESS), "vma flash memory failed");

}

void UniqueBuffer::download_data(void* dst, uint32_t size){
    auto dl_buffer = std::move(Core::UniqueBuffer::download(vulkan_context, size));

    auto buffer_copy_info = vk::BufferCopy()
        .setSrcOffset(0)
        .setDstOffset(0)
        .setSize(size);

    Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){
        cb.copyBuffer(
            this->get_raw_buffer(),
            dl_buffer.get_raw_buffer(),
            {buffer_copy_info}
        );
    });
    
    dl_buffer.write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* src = (uint8_t*)mapped_addr;
        
        std::memcpy(dst, src, size);
        
        return 0;
    });
}

UniqueBuffer::~UniqueBuffer(){
    if(buffer==VK_NULL_HANDLE)return;
    if(is_host_visible)unmap_memory();
    vmaDestroyBuffer(vulkan_context->vma_allocator, buffer, vma_allocation);
}

UniqueBuffer::UniqueBuffer(BOOST_RV_REF(UniqueBuffer) rhs){ //move constractor
    *this = std::move(rhs);
}         

UniqueBuffer& UniqueBuffer::operator=(BOOST_RV_REF(UniqueBuffer) rhs){ //move assignment
    if(this == &rhs){return *this;}
    if(this->buffer != VK_NULL_HANDLE) this->~UniqueBuffer(); 
    
    this->vulkan_context    = rhs.vulkan_context;
    this->buffer_size       = rhs.buffer_size;
    this->buffer            = rhs.buffer;
    this->device_address    = rhs.device_address;
    this->vma_allocation    = rhs.vma_allocation;
    this->mapped_memory     = rhs.mapped_memory;
    this->is_host_visible   = rhs.is_host_visible;
    rhs.buffer              = VK_NULL_HANDLE;
    return *this;
} 

//factory methods
UniqueBuffer UniqueBuffer::common(
    VulkanContext* vc, 
    size_t size, 
    bool host_visible, 
    vk::BufferUsageFlags usage,
    size_t alignment){
    
    VkBufferCreateInfo buffer_create_info = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(usage)
        .setSharingMode(vk::SharingMode::eExclusive);

    VmaAllocationCreateInfo vma_alloc_create_info = {}; //<-0初期化大事，これやらないと動かない
    vma_alloc_create_info.usage = VMA_MEMORY_USAGE_AUTO;
    if(host_visible){
        vma_alloc_create_info.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT;
        vma_alloc_create_info.requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT;
    }else{
        vma_alloc_create_info.requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    return std::move(UniqueBuffer(
        vc,
        size,
        buffer_create_info,
        vma_alloc_create_info,
        host_visible,
        alignment
    ));
}

UniqueBuffer UniqueBuffer::upload(
    VulkanContext* vc, 
    size_t size, 
    vk::BufferUsageFlags addtional_usage){
    
    VkBufferCreateInfo buffer_create_info = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(vk::BufferUsageFlagBits::eTransferSrc|addtional_usage)
        .setSharingMode(vk::SharingMode::eExclusive);
    
    VmaAllocationCreateInfo vma_alloc_create_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    };

    return std::move(UniqueBuffer(
        vc,
        size,
        buffer_create_info,
        vma_alloc_create_info,
        true,
        64
    ));
}

UniqueBuffer UniqueBuffer::download(
    VulkanContext* vc, 
    size_t size, 
    vk::BufferUsageFlags addtional_usage){
    
    VkBufferCreateInfo buffer_create_info = vk::BufferCreateInfo()
        .setSize(size)
        .setUsage(vk::BufferUsageFlagBits::eTransferDst|addtional_usage)
        .setSharingMode(vk::SharingMode::eExclusive);
    
    VmaAllocationCreateInfo vma_alloc_create_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT,
        .requiredFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
    };

    return std::move(UniqueBuffer(
        vc,
        size,
        buffer_create_info,
        vma_alloc_create_info,
        true,
        64
    ));
}

}