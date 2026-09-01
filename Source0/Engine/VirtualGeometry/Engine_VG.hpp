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

//vg
#include "../../VirtualGeometry/ClusterSelectPipeline.hpp"
#include "../../VirtualGeometry/ClusterSelectPipeline2.hpp"
#include "../../VirtualGeometry/ClearVGPipeline.hpp"
#include "../../VirtualGeometry/ClusterMeshPipeline.hpp"
#include "../../VirtualGeometry/VGDebugPipeline.hpp"
#include "../../VirtualGeometry/MatID2DepthPipeline.hpp"
#include "../../VirtualGeometry/MaterialTile.hpp"
#include "../../VirtualGeometry/MaterialClassify.hpp"
#include "../../VirtualGeometry/SoftRaster.hpp"
#include "../../VirtualGeometry/OcclusionHiz.hpp"

#include "../Include/ResourceRegistry.hpp"
#include "Editor.hpp"

#include <inih/cpp/INIReader.h>

#include <memory>
#include <chrono>


#define CLASSIFY_TILE_WIDTH (64)
#define CLASSIFY_THREAD_WIDTH (16)
#define CLASSIFY_MATERIAL_CHUNK_MAX (256)
#define MAX_MATERIAL (CLASSIFY_MATERIAL_CHUNK_MAX * 32)

namespace Irori::Engine::VirtualGeometry{
    
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
    
    // vg
    Irori::VirtualGeometry::UniqueClusterSelectPipeline cluster_select_pipeline;
    Irori::VirtualGeometry::UniqueClusterSelectPipeline2 cluster_select_pipeline2;
    Irori::VirtualGeometry::UniqueClearVGPipeline clear_vg_pipeline;
    Irori::VirtualGeometry::UniqueClusterMeshPipeline cluster_mesh_pipeline;
    Irori::VirtualGeometry::UniqueClusterMeshPipeline cluster_mesh_pipeline2;
    Irori::VirtualGeometry::UniqueVGDebugPipeline vg_debug_pipeline;
    Irori::VirtualGeometry::UniqueMatID2DepthPipeline matid2depth_pipeline;
    Irori::VirtualGeometry::UniqueMaterialClassifyPipeline material_classify_pipeline;
    Irori::VirtualGeometry::UniqueMaterialTilePipeline material_tile_pipeline;
    Irori::VirtualGeometry::UniqueSoftRasterPipeline soft_raster_pipeline;
    Irori::VirtualGeometry::UniqueSoftRasterPipeline soft_raster_pipeline2;
    Irori::VirtualGeometry::UniqueOcclusionHizPipeline occlusion_hiz_pipeline;
    uint32_t tile_num_x, tile_num_y, tile_num;
    
    // cpu frame render data
    Render::CPU_FrameRenderData cpu_frame_render_data;
    size_t current_frame_render_instance_reference_count = 0;
    
    // screen params
    uint32_t window_width = 1920, window_height = 1200;
    uint32_t render_target_width = 1920, render_target_height = 1200;
    
    // editor
    UniqueEditor editor;

    // occlusion hiz
    std::vector<std::string> occlusion_hiz_names;
    std::vector<vk::ImageView> occlusion_hiz;

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
    
    // helper
    void make_occlusion_hiz();
};







}