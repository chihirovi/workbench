#include "MaterialTile.hpp"

namespace Irori::VirtualGeometry{

UniqueMaterialTilePipeline::UniqueMaterialTilePipeline(
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
        .set_depth_attachment_info(&depth_image, std::nullopt)
        .set_depth_test_state(true, false, vk::CompareOp::eEqual);
    
    rasterize_pipeline_template = builder.build();
}

void UniqueMaterialTilePipeline::cmd_dispatch(
    vk::CommandBuffer& cb, 
    Core::UniqueBuffer& arg_buffer,
    Irori::Core::UniqueDescriptorSets& global_descsets){

    // begin renderpass -> bind pipeline -> bind descsets の順が推奨らしい
    rasterize_pipeline_template
        .cmd_begin_renderpass(cb)
        .cmd_bind_pipeline(cb)
        //.cmd_clear_attachments(cb)
        .cmd_bind_local_descsets(cb);

    global_descsets.bind_local_descriptor_sets(
        cb,
        *rasterize_pipeline_template.pipeline_layout,
        vk::PipelineBindPoint::eGraphics
    );
    
    //シェーディングモデルを変える場合，次の２種類の方法がある
    //１．シェーダー内で分岐させる（同じmaterial IDでの違うシェーディングモデルの可能性）
    //２．シェーダーを分ける（material IDとシェーディングモデルを１対１で対応付ける）

    //1は，マテリアルIDによるダイバージェンスの低減には効果はあるが，シェーダー内でのモデル分岐はコストが高い
    //２の場合，フラグメントシェーダーが変わり，レンダーパイプラインは複数できるため，indirect drawの呼び出しは複数回行う必要がある．
    /* つまりこう書かないといけない
    for(i=0; i<255; i++){
        pipeline[i].bind();
        cb.drawIndirect(
            arg_buffer.buffer, 		            // buffer
            sizeof(vk::DrawIndirectCommand)*i,  // offset
            1, 				                    // draw count
            sizeof(vk::DrawIndirectCommand)	    // stride
        );
    }
    */

    cb.drawIndirect(arg_buffer.buffer, 0, 256, sizeof(vk::DrawIndirectCommand));

    cb.endRenderPass(); 
}


}