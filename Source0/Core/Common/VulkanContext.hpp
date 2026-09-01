#pragma once
#include <stdio.h>

// vulkan hpp
// #define VULKAN_HPP_NO_DEFAULT_DISPATCHER
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1 
#include <vulkan/vulkan.hpp>

// vma 
#include "VmaImplemantation.hpp"

// other utility
#include <boost_1_88_0/boost/move/move.hpp>
#include "../../Utility/Error.hpp"
#include "../../Window/VkGlfwWindow.hpp"


namespace Irori::Core{

class VulkanContext{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(VulkanContext)

public:
    /*counter*/
    inline static uint32_t vulkan_context_counter = 0;

    /*vulkan instance and device*/
    vk::UniqueInstance instance;
    vk::PhysicalDevice physical_device;
    vk::UniqueDevice device;
    
    /*queue*/
    uint32_t queue_family_index = 0;
    vk::Queue queue; //graphics + compute
                     
    /*command*/
    vk::UniqueCommandPool cmd_pool;
    std::vector<vk::UniqueCommandBuffer> cmd_bufs;

    /*layer + Extentin + feature*/
    std::vector<const char *> inst_required_layers;
    std::vector<const char *> inst_required_extentions;

    std::vector<const char *> dev_required_layers;
    std::vector<const char *> dev_required_extentions;
    vk::PhysicalDeviceFeatures2 physical_device_features2 = {}; //1.0 features + pNext
    vk::PhysicalDeviceVulkan11Features physical_device_vulkan11_features = {}; //1.1 features
    vk::PhysicalDeviceVulkan12Features physical_device_vulkan12_features = {}; //1.2 features
    vk::PhysicalDeviceVulkan13Features physical_device_vulkan13_features = {}; //1.3 features
    void* extention_festures_pNext = NULL; 
    
    /*vk glfw window*/
    UniqueVkGlfwWindow* window = nullptr;
    vk::UniqueSurfaceKHR surface;
    vk::SurfaceFormatKHR surface_format; 
    vk::PresentModeKHR surface_present_mode;
    
    /*vma allocator*/
    VmaAllocator vma_allocator;

public:
    VulkanContext();
    virtual ~VulkanContext();
    void init();
    void create_instance(); 
    void pick_physical_device();
    void create_logical_device();
    void create_queue();
    void create_command_pool();
    void alloc_command_buffers(uint32_t cmd_buf_counts);
    void init_vma_allocator();
    bool is_physical_device_suitable(vk::PhysicalDevice pd);
    bool check_queue(vk::PhysicalDevice pd);
    bool check_device_extentions(vk::PhysicalDevice pd);
    bool check_device_features(vk::PhysicalDevice pd);
    
    //window surface
    void set_pWindow(UniqueVkGlfwWindow* w);
    void create_unique_surface();
    void check_surface_info();
    bool check_surface_formats(vk::Format format, vk::ColorSpaceKHR c_space);
    bool check_surface_present_modes(vk::PresentModeKHR mode);
    

    //instance configure
    VulkanContext& add_instance_layer(const char* name);
    VulkanContext& add_instance_extention(const char* name);

    //instance device configure
    VulkanContext& add_device_layer(const char* name); 
    VulkanContext& add_device_extention(const char* name); 
    vk::PhysicalDeviceFeatures& vulkan10_features();
    vk::PhysicalDeviceVulkan11Features& vulkan11_features();
    vk::PhysicalDeviceVulkan12Features& vulkan12_features();
    vk::PhysicalDeviceVulkan13Features& vulkan13_features();
    VulkanContext& set_p_extention_features_chain(void* p_extention_features_chain);
};

}