#include "VulkanContext.hpp"
VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE //only 1 can exist in program

namespace Irori::Core{
    
VulkanContext::VulkanContext(){};

VulkanContext::~VulkanContext(){
    vulkan_context_counter--;
    vmaDestroyAllocator(vma_allocator);
};

void VulkanContext::init(){
    vulkan_context_counter++;
    ASSERT_WITH_MSG((vulkan_context_counter==1), "vulkan Cntext can exist only 1 at a time");
    create_instance();       
    if(window != nullptr) create_unique_surface();
    pick_physical_device();   
    if(window != nullptr) check_surface_info();
    create_logical_device();
    create_queue();
    create_command_pool();
    alloc_command_buffers(8);
    init_vma_allocator();
    

}

void VulkanContext::create_instance(){
    auto app_info = vk::ApplicationInfo()
        .setPApplicationName("my_application")
        .setPEngineName("my_engine")
        .setApiVersion(VK_API_VERSION_1_3);

    auto inst_create_info = vk::InstanceCreateInfo()
        .setPApplicationInfo(&app_info)
        .setPEnabledLayerNames(inst_required_layers)
        .setPEnabledExtensionNames(inst_required_extentions);

    // 本当はLoaderを定義して init(loader) をする必要があるけど
    // そのままinit()すると，中で static loader が定義されて，
    // 代わりに init(loader) を行ってくれる. (定義を見ると早い)
    VULKAN_HPP_DEFAULT_DISPATCHER.init();

    instance = vk::createInstanceUnique(inst_create_info);

    ASSERT_WITH_MSG((instance?true:false), "create instance failed");

    VULKAN_HPP_DEFAULT_DISPATCHER.init( *instance );
}

void VulkanContext::pick_physical_device(){
    std::vector<vk::PhysicalDevice> physical_devices 
        = instance->enumeratePhysicalDevices();
         
    bool is_suitable_physic_dev_exist = false;

    for(auto pd : physical_devices){
        if(is_physical_device_suitable(pd)){
            physical_device = pd;
            is_suitable_physic_dev_exist = true;
            break;
        }
    }
    
    ASSERT_WITH_MSG(is_suitable_physic_dev_exist, "can't found suitable physical device");
}

void VulkanContext::create_logical_device(){

    std::vector<float> queue_priorities = {1.0f};
    std::vector<vk::DeviceQueueCreateInfo> queue_create_infos = {
        vk::DeviceQueueCreateInfo()
            .setQueueFamilyIndex(queue_family_index)
            .setQueuePriorities(queue_priorities)
    };
    //create structure chain
    physical_device_features2.pNext         = &physical_device_vulkan11_features;
    physical_device_vulkan11_features.pNext = &physical_device_vulkan12_features;
    physical_device_vulkan12_features.pNext = &physical_device_vulkan13_features;
    physical_device_vulkan13_features.pNext = extention_festures_pNext;
    
    auto dev_create_info = vk::DeviceCreateInfo()
        .setQueueCreateInfos(queue_create_infos)
        .setPEnabledLayerNames(dev_required_layers)
        .setPEnabledExtensionNames(dev_required_extentions)
        .setPNext(&physical_device_features2);
    
    device = physical_device.createDeviceUnique(dev_create_info);
    
    ASSERT_WITH_MSG((device?true:false), "create instance failed");

    VULKAN_HPP_DEFAULT_DISPATCHER.init( *device );
}

void VulkanContext::create_queue(){
    queue = device->getQueue(queue_family_index, 0);
    
    ASSERT_WITH_MSG((queue?true:false), "create queue failed");
}

void VulkanContext::create_command_pool(){
    cmd_pool = device->createCommandPoolUnique(
        vk::CommandPoolCreateInfo()
            .setQueueFamilyIndex(queue_family_index)
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
    );
}

void VulkanContext::alloc_command_buffers(uint32_t cmd_buf_counts){
    cmd_bufs = device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
            .setCommandPool(*cmd_pool)
            .setCommandBufferCount(cmd_buf_counts)
            .setLevel(vk::CommandBufferLevel::ePrimary)
    );
} 

void VulkanContext::init_vma_allocator(){
    
    VmaVulkanFunctions vk_funtions = {};
    vk_funtions.vkGetInstanceProcAddr  = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetInstanceProcAddr;
    vk_funtions.vkGetDeviceProcAddr    = VULKAN_HPP_DEFAULT_DISPATCHER.vkGetDeviceProcAddr;

    VmaAllocatorCreateInfo allocator_create_info = {
        .instance           = *instance,
        .physicalDevice     = physical_device,
        .device             = *device,
        .vulkanApiVersion   = VK_API_VERSION_1_3, 
        .pVulkanFunctions   = &vk_funtions,
    };

   if(physical_device_vulkan12_features.bufferDeviceAddress == vk::True)
        allocator_create_info.flags = VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
    
    auto result = vmaCreateAllocator(&allocator_create_info, &vma_allocator);
    ASSERT_WITH_MSG((result == VK_SUCCESS), "cant init vma allocator");
}

bool VulkanContext::is_physical_device_suitable(vk::PhysicalDevice pd){
    //check queue
    if(!check_queue(pd)) return false;
    
    //check extentions
    if(!check_device_extentions(pd)) return false;
    
    //check features
    if(!check_device_features(pd)) return false;
    
    //result
    return true;
}

bool VulkanContext::check_queue(vk::PhysicalDevice pd){
    std::vector<vk::QueueFamilyProperties> queue_props = pd.getQueueFamilyProperties();

    for(int i=0; i<queue_props.size(); i++){
        vk::QueueFamilyProperties queue_prop = queue_props[i];
        
        bool is_surface_supported = true;
        if(window!= nullptr){
            is_surface_supported = pd.getSurfaceSupportKHR(i,*surface);
        }

        if( queue_prop.queueFlags & vk::QueueFlagBits::eGraphics
            && queue_prop.queueFlags & vk::QueueFlagBits::eCompute
            && is_surface_supported){
            
            queue_family_index = i;//record queue index
            return true;
        }
    }
    return false;
}

bool VulkanContext::check_device_extentions(vk::PhysicalDevice pd){
    std::vector<vk::ExtensionProperties> ext_props = pd.enumerateDeviceExtensionProperties();

    for(auto req_ext : dev_required_extentions){
        for(size_t i=0; i<ext_props.size(); i++){
            if(std::string_view(req_ext) == std::string_view(ext_props[i].extensionName.data())){
                break;

            }else if(i == ext_props.size()-1){
                printf("don't support extention: %s\n", req_ext);
                return false;
            }
        }
    }       
    return true;
}

bool VulkanContext::check_device_features(vk::PhysicalDevice pd){
    //todo!
    return true;
}

void VulkanContext::set_pWindow(UniqueVkGlfwWindow* w){
    window = w;
}

void VulkanContext::create_unique_surface(){
    VkSurfaceKHR raw_surface = window->get_VkSurfaceKHR(instance);
    ASSERT_WITH_MSG(!(raw_surface==VK_NULL_HANDLE), "create surface faild");
    surface = vk::UniqueSurfaceKHR{raw_surface,*instance};
}

void VulkanContext::check_surface_info(){
    ASSERT_WITH_MSG(
        check_surface_formats(vk::Format::eR8G8B8A8Unorm, vk::ColorSpaceKHR::eSrgbNonlinear),
        "no suitable surface format"
    );

    ASSERT_WITH_MSG(
        check_surface_present_modes(vk::PresentModeKHR::eFifo),
        "no suitable surface present mode"
    );
}   

bool VulkanContext::check_surface_formats(vk::Format format, vk::ColorSpaceKHR c_space){
    auto surface_formats = physical_device.getSurfaceFormatsKHR(*surface);

    for(auto& iformat: surface_formats){
        if(iformat.format == format && iformat.colorSpace == c_space){
            surface_format = iformat;
            return true;
        }
    }

    return false;
}


bool VulkanContext::check_surface_present_modes(vk::PresentModeKHR mode){
    auto surface_present_modes = physical_device.getSurfacePresentModesKHR(*surface);

    for(auto& present_mode: surface_present_modes){
        if(present_mode == mode){
            surface_present_mode = present_mode;
            return true;
        }
    }
    
    return false;
}

VulkanContext& VulkanContext::add_instance_layer(const char* name){
    inst_required_layers.push_back(name);
    return *this;
}

VulkanContext& VulkanContext::add_instance_extention(const char* name){
    inst_required_extentions.push_back(name);
    return *this;
}

VulkanContext& VulkanContext::add_device_layer(const char* name){
    dev_required_layers.push_back(name);
    return *this;
}

VulkanContext& VulkanContext::add_device_extention(const char* name){
    dev_required_extentions.push_back(name);
    return *this;
}

vk::PhysicalDeviceFeatures& VulkanContext::vulkan10_features(){
    return this->physical_device_features2.features;
}
vk::PhysicalDeviceVulkan11Features& VulkanContext::vulkan11_features(){
    return this->physical_device_vulkan11_features;
}
vk::PhysicalDeviceVulkan12Features& VulkanContext::vulkan12_features(){
    return this->physical_device_vulkan12_features;
}
vk::PhysicalDeviceVulkan13Features& VulkanContext::vulkan13_features(){
    return this->physical_device_vulkan13_features;
}

VulkanContext& VulkanContext::set_p_extention_features_chain(void* p_extention_features_chain){
    extention_festures_pNext = p_extention_features_chain;
    return *this;
}

}