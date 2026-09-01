#include "ConfigureVulkanContext.hpp"

namespace Irori::Core{

void configure_vulkan_context_with_window(VulkanContext* vc, UniqueVkGlfwWindow* window){
    
    //instance layers
    vc->add_instance_layer("VK_LAYER_KHRONOS_validation");

    //instance extention
    if(window != nullptr){
        vc->set_pWindow(window);
        window->set_instance_extention(&(vc->inst_required_extentions));
    }
    vc->add_instance_extention(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    //device layer
    vc->add_instance_layer("VK_LAYER_KHRONOS_validation");

    //device extention
    if(window != nullptr){
        vc->add_device_extention(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }
    vc->add_device_extention(VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME);

    //for vk raytracing API
    vc->add_device_extention(VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME)
    .add_device_extention(VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME)
    .add_device_extention(VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);
    
    // mesh shader
    vc->add_device_extention(VK_EXT_MESH_SHADER_EXTENSION_NAME);

    //other
    vc->add_device_extention(VK_KHR_PIPELINE_LIBRARY_EXTENSION_NAME)
    .add_device_extention(VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME);

    //device features
    //create structure chain
    static vk::PhysicalDeviceMeshShaderFeaturesEXT mesh_shader_features;
    mesh_shader_features
        .setMeshShader(vk::True)
        .setTaskShader(vk::True);
    static vk::StructureChain features2_info_chain(         //dummy
        vk::PhysicalDeviceFeatures2                         {},//dummy
        vk::PhysicalDeviceRayTracingPipelineFeaturesKHR     {vk::True},
        vk::PhysicalDeviceAccelerationStructureFeaturesKHR  {vk::True},
        vk::PhysicalDeviceShaderImageAtomicInt64FeaturesEXT {vk::True},
        mesh_shader_features
    );

    vc->set_p_extention_features_chain(
        features2_info_chain.get<vk::PhysicalDeviceFeatures2>().pNext //dummy.pNext
    );

    //1.0 features
    vc->vulkan10_features()
        .setShaderInt64(vk::True)
        .setMultiDrawIndirect(vk::True)
        .setIndependentBlend(vk::True)
        .setShaderInt16(vk::True)
        .setShaderFloat64(vk::True)
        .setFragmentStoresAndAtomics(vk::True)
        .setVertexPipelineStoresAndAtomics(vk::True)
        .setSamplerAnisotropy(vk::True);
    
    //1.1 features
    vc->vulkan11_features()
        .setShaderDrawParameters(vk::True)
        .setUniformAndStorageBuffer16BitAccess(vk::True)
        .setStorageBuffer16BitAccess(vk::True);
    
    //1.2 features
    vc->vulkan12_features()
        //.setUniformBufferStandardLayout(vk::True)
        .setStorageBuffer8BitAccess(vk::True)
        .setUniformAndStorageBuffer8BitAccess(vk::True)
        .setShaderInt8(vk::True)
        .setShaderFloat16(vk::True)
        .setTimelineSemaphore(vk::True)
        .setDrawIndirectCount(vk::True)
        .setBufferDeviceAddress(vk::True)
        .setDescriptorIndexing(vk::True)
        .setDescriptorBindingPartiallyBound(vk::True)
        .setRuntimeDescriptorArray(vk::True)
        .setShaderSampledImageArrayNonUniformIndexing(vk::True)
        .setScalarBlockLayout(vk::True); //uniform, storage, 両方にscalar layoutを許可する, ついでにstd430も許可となる

    //1.3 features
    vc->vulkan13_features()
        .setShaderIntegerDotProduct(vk::True)
        .setSynchronization2(vk::True);
}

}