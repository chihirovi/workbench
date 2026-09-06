#include "MaterialClassify.hpp"


namespace Irori::VirtualGeometry{

UniqueMaterialClassifyPipeline::UniqueMaterialClassifyPipeline(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info){
    
    comp_pipeline_templtate = Core::UniqueCompPipelineTemplate(
        vc,
        pipeline_shader_resource,
        global_desc_info
    );
}

void UniqueMaterialClassifyPipeline::cmd_dispatch(
    vk::CommandBuffer& cb, 
    uint32_t tile_num_x,
    uint32_t tile_num_y,
    Irori::Core::UniqueDescriptorSets& global_descsets){

    //bind pipeline -> bind descsetsの順が推奨らしい
    comp_pipeline_templtate
        .bind_pipeline(cb)
        .bind_local_descsets(cb);
    
    global_descsets.bind_local_descriptor_sets(
        cb,
        *comp_pipeline_templtate.pipeline_layout,
        vk::PipelineBindPoint::eCompute
    );

    cb.dispatch(tile_num_x, tile_num_y, 1);
}


}