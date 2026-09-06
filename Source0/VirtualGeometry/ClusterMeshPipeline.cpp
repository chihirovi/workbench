#include "ClusterMeshPipeline.hpp"

namespace Irori::VirtualGeometry{

UniqueClusterMeshPipeline::UniqueClusterMeshPipeline(
    Core::VulkanContext* vc, 
    uint32_t w, uint32_t h,
    Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
    Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info){
    
    uint32_t width = w;
    uint32_t height = h;
       
    // build
    Core::UniqueRasterPipelineTemplate::Builder builder(vc);
    builder
        .set_w_h(width, height)
        .set_shader_desc_info(pipeline_shader_resource, global_desc_info)
        .set_rasterizer_state(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        //.set_rasterizer_state(vk::PolygonMode::eFill, vk::CullModeFlagBits::eBack, vk::FrontFace::eCounterClockwise)
        .set_depth_test_state(false, false, vk::CompareOp::eLessOrEqual);
    
    rasterize_pipeline_template = builder.build();
}

void UniqueClusterMeshPipeline::cmd_dispatch(
    vk::CommandBuffer& cb, 
    Core::UniqueBuffer& indirect_buffer,
    Stage stage,
    Irori::Core::UniqueDescriptorSets& global_descsets){

    // begin renderpass -> bind pipeline -> bind descsets の順が推奨らしい
    rasterize_pipeline_template
        .cmd_begin_renderpass(cb)
        .cmd_bind_pipeline(cb)
        .cmd_bind_local_descsets(cb);

    global_descsets.bind_local_descriptor_sets(
        cb,
        *rasterize_pipeline_template.pipeline_layout,
        vk::PipelineBindPoint::eGraphics
    );
    
    //cb.drawMeshTasksEXT(opaque_inst_count, 1, 1);
    cb.drawMeshTasksIndirectEXT(indirect_buffer.buffer, uint32_t(stage), 1, 0);

    cb.endRenderPass(); 
}


}