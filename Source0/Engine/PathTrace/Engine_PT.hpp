#pragma once
#include "../../Core/Common/VulkanContext.hpp"
#include "../../Core/Common/ConfigureVulkanContext.hpp"
#include "../../Core/Common/Buffer.hpp"
#include "../../Core/Common/Image.hpp"
#include "../../Core/Common/ShaderReflect.hpp"
#include "../../Core/Common/DescriptorSets.hpp"
#include "../../Core/Common/ShaderModule.hpp"
#include "../../Core/Common/Barrier2.hpp"
#include "../../Core/Common/Swapchain.hpp"
#include "../../Core/Common/Sampler.hpp"

#include "../../Render/RenderLoop.hpp"
#include "../../Scene/SceneManager.hpp"
#include "../../Scene/SceneAsManager.hpp"

#include "../../PathTrace/PtPipeline.hpp"

#include "../Include/ResourceRegistry.hpp"
#include "Editor.hpp"
#include "CalcLightTileInfo.hpp"

#include <inih/cpp/INIReader.h>

#include <memory>
#include <chrono>

namespace Irori::Engine::PathTrace{
    
class UniqueEngine{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueEngine);

public:
    // common resource
    UniqueVkGlfwWindow window;
    Core::VulkanContext vulkan_context;
    Core::UniqueSwapchain swapchain;
    Core::UniqueSamplerPool sampler_pool;
    UniqueResourceRegistry<Core::UniqueBuffer> buffer_registry;
    UniqueResourceRegistry<Core::UniqueImage> image_registry;
    Core::UniqueDescriptorSets global_descriptor_sets;
    
    // scene
    Scene::UniqueSceneManager scene_manager;
    Scene::UniqueSceneAsManager scene_as_manager;
    
    // path trace
    Irori::PathTrace::UniquePtPipeline pt_pipeline;
    
    // cpu frame render data
    Render::CPU_FrameRenderData cpu_frame_render_data;
    size_t current_frame_render_instance_reference_count = 0;
    
    // screen params
    uint32_t window_width = 1920, window_height = 1200;
    uint32_t render_target_width = 1920, render_target_height = 1200;
    
    // editor
    UniqueEditor editor;

    // light polygon info
    std::vector<float> light_tile_info;
    
public:
    UniqueEngine(BOOST_RV_REF(UniqueEngine) rhs) = default;               //move constractor
    UniqueEngine& operator=(BOOST_RV_REF(UniqueEngine) rhs) = default;    //move assignment

    UniqueEngine(std::string config_file);
    void init_base(std::string& config_file);
    void init_buffers();
    void init_images();
    void init_pipelines();
    void cpu_frame_render_data_to_staging_buffers();
    void run();
};







}