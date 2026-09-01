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

#include "../../Deferred/OpaqueMeshPipeline.hpp"
#include "../../Deferred/MaskMeshPipeline.hpp"
#include "../../Deferred/OpaqueVertexPipeline.hpp"
#include "../../Deferred/MaskVertexPipeline.hpp"
#include "../../PBR/PbrPipeline.hpp"

#include "../../Render/RenderLoop.hpp"
#include "../../Scene/SceneManager.hpp"
#include "../../Scene/SceneAsManager.hpp"
#include "../../Scene/SceneAsLodManager.hpp"

#include "../Include/ResourceRegistry.hpp"
//#include "../Include/ExportImage.hpp"
#include "Editor.hpp"

#include "../../DDGI/DdgiDesc.hpp"
#include "../../DDGI/ProbeTracePipeline.hpp"
#include "../../DDGI/BlendPipeline.hpp"
#include "../../DDGI/IrradianceShadowPipeline.hpp"
#include "../../DDGI/ProbeVisualizePipeline.hpp"
#include "../../DDGI/AsLodVisualizePipeline.hpp"
#include "../../DDGI/RelocationPipeline.hpp"

#include <inih/cpp/INIReader.h>

#include <memory>
#include <chrono>

namespace Irori::Engine::DDGI_LOD{
    
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
    //Scene::UniqueSceneAsManager scene_as_manager;
    Scene::UniqueSceneAsLodManager scene_as_lod_manager;
    Scene::SceneAsModelLodFile as_lod_files;
    
    // base pass pipelines
    Deferred::UniqueOpaqueMeshPipeline deferred_opaque_mesh_pipeline;
    Deferred::UniqueMaskMeshPipeline deferred_mask_mesh_pipeline;
    Deferred::UniqueOpaqueVertexPipeline deferred_opaque_vertex_pipeline;
    Deferred::UniqueMaskVertexPipeline deferred_mask_vertex_pipeline;
    Pbr::UniquePbrPipeline pbr_pipeline;
    
    // ddgi 
    DDGI::DdgiDesc ddgi_desc;
    DDGI::UniqueProbeTracePipeline probe_trace_pipeline;
    DDGI::UniqueBlendPipeline irradiance_blend_pipeline;
    DDGI::UniqueBlendPipeline chit_t_blend_pipeline;
    DDGI::UniqueIrradianceShadowPipeline irradiance_shadow_pipeline;
    DDGI::UniqueProbeVisPipeline probe_vis_pipeline;
    DDGI::UniqueAsLodVisualizePipeline as_lod_vis_pipeline;
    DDGI::UniqueRelocationPipeline relocation_pipeline;
    //uint32_t probe_num_ray = DDGI_PROBE_NUM_RAY;
    uint32_t probe_num_ray = 256;
    //uint32_t probe_num_ray = 192;
    //uint32_t probe_num_ray = 128;
    //uint32_t probe_num_ray = 64;
    
    // cpu frame render data
    Render::CPU_FrameRenderData cpu_frame_render_data;
    size_t current_frame_render_instance_reference_count = 0;
    
    // screen params
    uint32_t window_width = 1920, window_height = 1200;
    uint32_t render_target_width = 1920, render_target_height = 1200;
    
    // editor
    UniqueEditor editor;

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
    
    void create_scene_0();
    void create_scene_1();
    void create_scene_2();
    void create_scene_3();
};

}


















