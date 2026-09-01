#pragma once
#include "VulkanContext.hpp"
#include "Image.hpp"

namespace Irori::Core{

class UniqueSwapchain{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueSwapchain);

public:
    VulkanContext* vulkan_context = nullptr;
    vk::UniqueSwapchainKHR swapchain;
    uint32_t img_idx;
    std::vector<UniqueImage> swapchain_images;
    
public:
    UniqueSwapchain(){;}
    UniqueSwapchain(VulkanContext* vc);
    UniqueSwapchain(BOOST_RV_REF(UniqueSwapchain) rhs) = default;              //move constractor
    UniqueSwapchain& operator=(BOOST_RV_REF(UniqueSwapchain) rhs) = default;    //move assignment
                                                                            
    void acquire_next_image(vk::Semaphore& next_image_ready_semaphore);
    vk::ImageView next_raw_image_view(){return *swapchain_images[img_idx].image_view;}
    vk::Image next_raw_image(){return swapchain_images[img_idx].image;}
    UniqueImage& next_image(){return swapchain_images[img_idx];}
    uint32_t next_image_index(){return img_idx;}
    void presentation(vk::Semaphore& wait_semaphore);
};

    
}