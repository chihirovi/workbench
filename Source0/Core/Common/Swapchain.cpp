#include "Swapchain.hpp"

namespace Irori::Core{
    
UniqueSwapchain::UniqueSwapchain(VulkanContext* vc){

    vulkan_context = vc;

    // get surface info
    auto surface_capabilities = vulkan_context->physical_device
        .getSurfaceCapabilitiesKHR(*vulkan_context->surface);
    
    //どうせ，computeでライティングやらガンマ補正やらするし，
    //たいていの場合はこれで事足りるはず
    vk::ImageUsageFlags image_usages = 
        vk::ImageUsageFlagBits::eStorage|
        vk::ImageUsageFlagBits::eTransferDst|
        vk::ImageUsageFlagBits::eColorAttachment;

    // create swapchain
    swapchain = vulkan_context->device->createSwapchainKHRUnique(
        vk::SwapchainCreateInfoKHR()
            .setSurface(vulkan_context->surface.get())
            .setMinImageCount(surface_capabilities.minImageCount+1)
            .setImageFormat(vulkan_context->surface_format.format)
            .setImageColorSpace(vulkan_context->surface_format.colorSpace)
            .setImageExtent(surface_capabilities.currentExtent)
            .setImageArrayLayers(1)
            .setImageUsage(image_usages)
            .setImageSharingMode(vk::SharingMode::eExclusive)
            .setPreTransform(surface_capabilities.currentTransform)
            .setPresentMode(vulkan_context->surface_present_mode)
            .setClipped(VK_TRUE)
            .setQueueFamilyIndices(vulkan_context->queue_family_index)
    );

    ASSERT_WITH_MSG((!!swapchain), "Swapchain : can't create swapchain");
    
    // create swapchain images
    auto swapchain_raw_images = vulkan_context->device->getSwapchainImagesKHR(*swapchain);
    for(auto& swapchain_raw_image : swapchain_raw_images){

        auto temp_image = UniqueImage::Builder2D(vulkan_context)
            .set_size_format_usage_layout(
                surface_capabilities.currentExtent.width,
                surface_capabilities.currentExtent.height,
                vulkan_context->surface_format.format,
                image_usages,
                vk::ImageLayout::ePresentSrcKHR
            )
            .set_raw_image(swapchain_raw_image)
            .build();

        swapchain_images.push_back(std::move(temp_image));
    }
}

void UniqueSwapchain::acquire_next_image(vk::Semaphore& next_image_ready_semaphore){
    auto result = vulkan_context->device->acquireNextImageKHR(  
        *swapchain, 
        1'000'000'000,//1second(ns)
        next_image_ready_semaphore
    );

    ASSERT_WITH_MSG((result.result == vk::Result::eSuccess), "cant get next frame.");

    img_idx = result.value;
}

void UniqueSwapchain::presentation(vk::Semaphore& wait_semaphore){
    auto result = vulkan_context->queue.presentKHR(
        vk::PresentInfoKHR()
            .setSwapchainCount(1)
            .setPSwapchains(&swapchain.get()) 
            .setPImageIndices(&img_idx)
            .setWaitSemaphoreCount(1)
            .setPWaitSemaphores(&wait_semaphore)
    );
    
    ASSERT_WITH_MSG((result == vk::Result::eSuccess), "faild to present image");
}


}
