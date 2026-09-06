#include "MatID2DepthPipeline.hpp"

namespace Irori::VirtualGeometry{

UniqueMatID2DepthPipeline::UniqueMatID2DepthPipeline(
    Core::VulkanContext* vc, 
    Irori::Core::UniqueImage& depth_image,
    Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
    Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info){
    
    uint32_t width = depth_image.image_create_info.extent.width;
    uint32_t height = depth_image.image_create_info.extent.height;
       
    // build
    Core::UniqueRasterPipelineTemplate::Builder builder(vc);
    builder
        .set_w_h(width, height)
        .set_shader_desc_info(pipeline_shader_resource, global_desc_info)
        .set_rasterizer_state(vk::PolygonMode::eFill, vk::CullModeFlagBits::eNone, vk::FrontFace::eCounterClockwise)
        .set_depth_attachment_info(&depth_image, 1.0f)
        .set_depth_test_state(true, true, vk::CompareOp::eAlways);
    
    rasterize_pipeline_template = builder.build();
}

void UniqueMatID2DepthPipeline::cmd_dispatch(
    vk::CommandBuffer& cb, 
    Irori::Core::UniqueDescriptorSets& global_descsets){

    // begin renderpass -> bind pipeline -> bind descsets の順が推奨らしい
    rasterize_pipeline_template
        .cmd_begin_renderpass(cb)
        .cmd_bind_pipeline(cb)
        .cmd_clear_attachments(cb)
        .cmd_bind_local_descsets(cb);

    global_descsets.bind_local_descriptor_sets(
        cb,
        *rasterize_pipeline_template.pipeline_layout,
        vk::PipelineBindPoint::eGraphics
    );
    
    cb.draw(6, 1, 0, 0);

    cb.endRenderPass(); 
}


}