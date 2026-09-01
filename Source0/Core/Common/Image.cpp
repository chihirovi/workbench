#include "Image.hpp"
#include "Buffer.hpp"
#include "Submit.hpp"
#include "Barrier2.hpp"

namespace Irori::Core {

UniqueImage::UniqueImage(){;}

UniqueImage::UniqueImage(VulkanContext* vc){vulkan_context = vc;}

//destractor
UniqueImage::~UniqueImage(){
    if(image == VK_NULL_HANDLE) return;
    if(vma_allocation != nullptr){
        vmaDestroyImage(
            vulkan_context->vma_allocator,
            image,
            vma_allocation
        );
    }
    image = VK_NULL_HANDLE;
    vma_allocation = nullptr;
    image_view.reset();//デフォルトのデストラクタはnullでリセットしないので，ここでnull代入+解放
    /*[証拠]
    * printf("0 %32lx\n", VkImageView(image_view.get()));
    * image_view.~UniqueHandle();
    * printf("1 %32lx\n", VkImageView(image_view.get()));
    * 上と下でハンドル値が変わらない
    */
}

//move constractor
UniqueImage::UniqueImage(BOOST_RV_REF(UniqueImage) rhs){
    *this = std::move(rhs);
}

//move assignment
UniqueImage& UniqueImage::operator=(BOOST_RV_REF(UniqueImage) rhs){
    if(this == &rhs) return *this; 
    if(this->image != VK_NULL_HANDLE) this->~UniqueImage();

    this->vulkan_context                = rhs.vulkan_context;
    this->image                         = rhs.image;
    this->image_view                    = std::move(rhs.image_view);
    this->vma_allocation                = rhs.vma_allocation;
    this->current_mip0_array0_layout    = rhs.current_mip0_array0_layout;
    this->image_create_info             = rhs.image_create_info;
    this->image_view_create_info        = rhs.image_view_create_info;

    rhs.image                           = VK_NULL_HANDLE;
    rhs.vma_allocation                  = nullptr;

    return *this;
}

void UniqueImage::create_image(
    vk::ImageCreateInfo _image_create_info, 
    VmaAllocationCreateInfo _vma_alloc_create_info){

    image_create_info = _image_create_info;
    current_mip0_array0_layout = image_create_info.initialLayout;

    //create image
    ASSERT_WITH_MSG((image == VK_NULL_HANDLE), "image build failed, this image isn't NULL HANDLE");
    VkImageCreateInfo Vk_image_create_info = image_create_info;
    auto res = vmaCreateImage(
        vulkan_context->vma_allocator,
        &Vk_image_create_info,
        &_vma_alloc_create_info,
        &image,
        &vma_allocation,
        nullptr
    );
    ASSERT_WITH_MSG((res == VK_SUCCESS), "create image faild");
}

void UniqueImage::create_image_view(vk::ImageViewCreateInfo _image_view_create_info){
    image_view_create_info = _image_view_create_info;
    image_view = vulkan_context->device->createImageViewUnique(image_view_create_info);
    ASSERT_WITH_MSG((image_view?true:false), "create instance failed");
}

void UniqueImage::transter_data_to_mip0_array0(void* data){

    int w = image_create_info.extent.width;
    int h = image_create_info.extent.height;

    size_t size = w*h*bytes_from_format(image_create_info.format);
    auto staging_buffer = UniqueBuffer::upload(vulkan_context, size);
    staging_buffer.transfer_data(data, size);

    transter_data_to_mipN_array0(staging_buffer, 0);
}

void UniqueImage::transter_data_to_mipN_array0(Core::UniqueBuffer& staging_buffer, uint32_t mip_level){

    auto width  = std::max((int32_t)(this->image_create_info.extent.width >> mip_level), 1);
    auto height = std::max((int32_t)(this->image_create_info.extent.height >> mip_level), 1);

    vk::BufferImageCopy img_copy_region;
    img_copy_region
        .setBufferOffset(0)
        .setImageSubresource(
            vk::ImageSubresourceLayers()
                .setAspectMask(image_view_create_info.subresourceRange.aspectMask)
                .setMipLevel(mip_level)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
        )
        .setImageOffset(vk::Offset3D(0,0,0))
        .setImageExtent(vk::Extent3D(width, height, 1))
        .setBufferRowLength(0)
        .setBufferImageHeight(0);

    auto original_layout = current_mip0_array0_layout;

    ImageBarrier::convert_image_layout(
        vulkan_context, 
        this->get_raw_image(),
        this->current_mip0_array0_layout,
        vk::ImageLayout::eTransferDstOptimal, 
        vk::ImageSubresourceRange()
            .setAspectMask(image_view_create_info.subresourceRange.aspectMask)
            .setBaseMipLevel(mip_level)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );

    Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){
        cb.copyBufferToImage(
            staging_buffer.get_raw_buffer(),
            this->get_raw_image(),
            vk::ImageLayout::eTransferDstOptimal,
            {img_copy_region}
        );
    });

    ImageBarrier::convert_image_layout(
        vulkan_context, 
        this->get_raw_image(),
        vk::ImageLayout::eTransferDstOptimal, 
        original_layout,
        vk::ImageSubresourceRange()
            .setAspectMask(image_view_create_info.subresourceRange.aspectMask)
            .setBaseMipLevel(mip_level)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );   
}

void UniqueImage::create_mipmap_from_mip0(){

    uint32_t width  = image_create_info.extent.width;
    uint32_t height = image_create_info.extent.height;

    ImageBarrier::convert_image_layout(
        vulkan_context, 
        this->get_raw_image(),
        this->current_mip0_array0_layout,
        vk::ImageLayout::eTransferSrcOptimal, 
        vk::ImageSubresourceRange()
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );
    
    for(uint32_t i=1; i<image_create_info.mipLevels; i++){
        //blit config
        vk::ImageBlit image_blit;

        //src
        image_blit.srcSubresource
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(i-1);
        image_blit.srcOffsets[1].x = (int32_t)(width    >> (i-1));
        image_blit.srcOffsets[1].y = (int32_t)(height   >> (i-1));
        image_blit.srcOffsets[1].z = 1;
        
        //dst
        image_blit.dstSubresource
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(i);
        image_blit.dstOffsets[1].x = (int32_t)(width    >> i);
        image_blit.dstOffsets[1].y = (int32_t)(height   >> i);
        image_blit.dstOffsets[1].z = 1;

        //submit
        Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){

            CmdImageBarrier::barrier(
                cb, 
                vk::AccessFlagBits2::eTransferRead,
                vk::AccessFlagBits2::eTransferWrite,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eTransfer,
                this->current_mip0_array0_layout,
                vk::ImageLayout::eTransferDstOptimal,
                this->get_raw_image(),
                vk::ImageSubresourceRange()
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseMipLevel(i)
                    .setLevelCount(1)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)               
            );

            cb.blitImage(
                image, vk::ImageLayout::eTransferSrcOptimal,
                image, vk::ImageLayout::eTransferDstOptimal,
                image_blit,
                vk::Filter::eLinear
            );

            CmdImageBarrier::barrier(
                cb, 
                vk::AccessFlagBits2::eTransferWrite,
                vk::AccessFlagBits2::eTransferRead,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::PipelineStageFlagBits2::eTransfer,
                vk::ImageLayout::eTransferDstOptimal,
                vk::ImageLayout::eTransferSrcOptimal,
                this->get_raw_image(),
                vk::ImageSubresourceRange()
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseMipLevel(i)
                    .setLevelCount(1)
                    .setBaseArrayLayer(0)
                    .setLayerCount(1)               
            );
        });
    }

    //layoutを元に戻す
    ImageBarrier::convert_image_layout(
        vulkan_context, 
        this->get_raw_image(),
        vk::ImageLayout::eTransferSrcOptimal, 
        current_mip0_array0_layout,
        vk::ImageSubresourceRange()
            .setAspectMask(image_view_create_info.subresourceRange.aspectMask)
            .setBaseMipLevel(0)
            .setLevelCount(image_create_info.mipLevels)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
    );
}


void UniqueImage::cmd_copy_to_image(vk::CommandBuffer& cb, UniqueImage& dst_image){
    ASSERT_WITH_MSG((this->image_create_info.extent == dst_image.image_create_info.extent),"size of copy src and dst is different");

    vk::ImageCopy imageCopyRegion;
    imageCopyRegion.srcSubresource.aspectMask       = this->image_view_create_info.subresourceRange.aspectMask;
    imageCopyRegion.srcSubresource.mipLevel         = this->image_view_create_info.subresourceRange.baseMipLevel;
    imageCopyRegion.srcSubresource.baseArrayLayer   = this->image_view_create_info.subresourceRange.baseArrayLayer;
    imageCopyRegion.srcSubresource.layerCount       = this->image_view_create_info.subresourceRange.layerCount;
    imageCopyRegion.srcOffset                       = vk::Offset3D{0, 0, 0};
    //imageCopyRegion.dstSubresource.aspectMask       = dst_image.image_view_create_info.subresourceRange.aspectMask;
    //imageCopyRegion.dstSubresource.mipLevel         = dst_image.image_view_create_info.subresourceRange.baseMipLevel;
    //imageCopyRegion.dstSubresource.baseArrayLayer   = dst_image.image_view_create_info.subresourceRange.baseArrayLayer;
    //imageCopyRegion.dstSubresource.layerCount       = dst_image.image_view_create_info.subresourceRange.layerCount;
    imageCopyRegion.dstSubresource.aspectMask       = this->image_view_create_info.subresourceRange.aspectMask;
    imageCopyRegion.dstSubresource.mipLevel         = this->image_view_create_info.subresourceRange.baseMipLevel;
    imageCopyRegion.dstSubresource.baseArrayLayer   = this->image_view_create_info.subresourceRange.baseArrayLayer;
    imageCopyRegion.dstSubresource.layerCount       = this->image_view_create_info.subresourceRange.layerCount;
    imageCopyRegion.dstOffset                       = vk::Offset3D{0, 0, 0};
    imageCopyRegion.extent                          = image_create_info.extent;

    cb.copyImage(
        image,
        vk::ImageLayout::eTransferSrcOptimal,
        dst_image.get_raw_image(),
        vk::ImageLayout::eTransferDstOptimal,
        {imageCopyRegion}
    );
}

//---------------
//builder methods
//---------------
UniqueImage UniqueImage::Builder2D::build(){
    ASSERT_TRUE_WITH_MSG((x == 0), "x size can't be 0");
    ASSERT_TRUE_WITH_MSG((y == 0), "y size can't be 0");
    ASSERT_TRUE_WITH_MSG((format == vk::Format::eUndefined), "format cat't be endefined");
    ASSERT_TRUE_WITH_MSG((init_layout == vk::ImageLayout::eUndefined), "init layout can't be Undefined");
    
    //mipmap
    uint32_t mip_levels = is_enable_mipmap ? (uint32_t)(floor(log2(std::max(x, y))) + 1) : 1;
    if(max_mip_levels != 1){
        mip_levels = max_mip_levels;
    }

    //usage
    if(is_enable_mipmap){
        usages |= vk::ImageUsageFlagBits::eTransferDst;
        usages |= vk::ImageUsageFlagBits::eTransferSrc;
    }
    if(data != nullptr){
        usages |= vk::ImageUsageFlagBits::eTransferDst;
    }

    //create image
    auto image_create_info = vk::ImageCreateInfo()
        .setExtent(vk::Extent3D(x,y,1))
        .setFormat(format)
        .setUsage(usages)
        .setImageType(vk::ImageType::e2D)
        .setMipLevels(mip_levels)
        .setArrayLayers(array_size)
        .setTiling(vk::ImageTiling::eOptimal)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSamples(sample_count);

    VmaAllocationCreateInfo vma_alloc_create_info = {
        .usage = VMA_MEMORY_USAGE_AUTO,
        .requiredFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
    };
    
    UniqueImage unique_image(vulkan_context);
    if(raw_image == VK_NULL_HANDLE){
        unique_image.create_image(image_create_info, vma_alloc_create_info);
    }else{
        unique_image.image = raw_image;
        unique_image.image_create_info = image_create_info;
        unique_image.current_mip0_array0_layout = image_create_info.initialLayout;
    }
    
    
    //create image view
    auto image_view_create_info = vk::ImageViewCreateInfo()
        .setImage(unique_image.get_raw_image())
        .setFormat(image_create_info.format)
        .setViewType(array_size == 1 ? vk::ImageViewType::e2D : vk::ImageViewType::e2DArray)
        .setComponents(
            vk::ComponentMapping()
                .setR(vk::ComponentSwizzle::eR)
                .setG(vk::ComponentSwizzle::eG)
                .setB(vk::ComponentSwizzle::eB)
                .setA(vk::ComponentSwizzle::eA)
        )
        .setSubresourceRange(
            vk::ImageSubresourceRange()
                .setAspectMask(image_view_aspect_from_format(format))
                .setLevelCount(image_create_info.mipLevels)         //number of mipmap
                .setLayerCount(image_create_info.arrayLayers)       //array size
                .setBaseMipLevel(0)                                 //mip level begin
                .setBaseArrayLayer(0)                               //array begin 
        );
    
    unique_image.create_image_view(image_view_create_info);
    
    //convert layout
    ImageBarrier::convert_image_layout(vulkan_context, unique_image, init_layout);
    
    //transter data
    if(data != nullptr){
        unique_image.transter_data_to_mip0_array0(data);
        //create mipmap
        if(is_enable_mipmap) unique_image.create_mipmap_from_mip0();
    }

    return std::move(unique_image);
}

//---------------
//builder methods
//---------------
UniqueImage UniqueImage::Builder2D_DDS::build(){
    usages |= vk::ImageUsageFlagBits::eTransferDst;
    auto image = Builder2D(vulkan_context)
                .set_size_format_usage_layout(dds_vk_data->width, dds_vk_data->height, dds_vk_data->format, usages, init_layout)
                .set_max_mip_level(dds_vk_data->numMips)
                .build();
    
    // staging buffer
    auto staging_buffer = UniqueBuffer::upload(vulkan_context, dds_vk_data->mipmaps[0].size_bytes());

    // transfer data
    for(uint32_t mip_level = 0; mip_level < dds_vk_data->numMips; mip_level++){

        dds::span<const uint8_t> data_span = dds_vk_data->mipmaps[mip_level];

        // data to staging buffer
        staging_buffer.transfer_data((void*)data_span.data(), data_span.size_bytes());
        //
        image.transter_data_to_mipN_array0(staging_buffer, mip_level);
    }

    return std::move(image);
}

//utility
vk::ImageAspectFlagBits image_view_aspect_from_format(vk::Format _format){
    switch (_format) {
        case vk::Format::eR8G8B8A8Unorm:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR8G8B8A8Srgb:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR16G16B16A16Snorm:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR32Sfloat:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR16G16B16A16Sfloat:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR32G32B32A32Sfloat:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eB10G11R11UfloatPack32:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR16G16Sfloat:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR32G32Sfloat:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR64Uint:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR32Uint:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eR8G8Unorm:
            return vk::ImageAspectFlagBits::eColor;
        case vk::Format::eD32Sfloat:
            return vk::ImageAspectFlagBits::eDepth;
        default :
            //ASSERT_WITH_MSG(false, "image_view_aspect_from_format: unknown format");
            return vk::ImageAspectFlagBits::eColor;
    };
}

uint32_t bytes_from_format(vk::Format _format){
    switch (_format) {
        case vk::Format::eR8G8B8A8Unorm:
            return 1*4; //bytes * num
        case vk::Format::eR8G8B8A8Srgb:
            return 1*4;
        case vk::Format::eR16G16B16A16Snorm:
            return 2*4;
        case vk::Format::eR32Sfloat:
            return 4*1;
        case vk::Format::eR32G32B32A32Sfloat:
            return 4*4;
        case vk::Format::eR16G16B16A16Sfloat:
            return 2*4;
        case vk::Format::eD32Sfloat:
            return 4*1;
        case vk::Format::eB10G11R11UfloatPack32:
            return 4;
        case vk::Format::eR16G16Sfloat:
            return 4;
        case vk::Format::eR32G32Sfloat:
            return 4*2;
        case vk::Format::eR64Uint:
            return 8;
        case vk::Format::eR32Uint:
            return 4;
        case vk::Format::eR8G8Unorm:
            return 1*2;
        default :
            ASSERT_WITH_MSG(false, "bytes from format: unknown format");
            return 0;
    }; 
}

}