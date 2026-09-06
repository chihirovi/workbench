#include "ClusterSelectPipeline2.hpp"


namespace Irori::VirtualGeometry{

UniqueClusterSelectPipeline2::UniqueClusterSelectPipeline2(
        Core::VulkanContext* vc,
        Irori::Core::ShaderReflect::PipelineShaderResourceInfo& pipeline_shader_resource, 
        Irori::Core::ShaderReflect::DescsetsInfo& global_desc_info){
    
    comp_pipeline_templtate = Core::UniqueCompPipelineTemplate(
        vc,
        pipeline_shader_resource,
        global_desc_info
    );
}

void UniqueClusterSelectPipeline2::cmd_dispatch(
    vk::CommandBuffer& cb, 
    Core::UniqueBuffer& arg_buffer, 
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
    int offset = (4*4 + 2) * sizeof(uint32_t);
    cb.dispatchIndirect(arg_buffer.get_raw_buffer(), offset);
}


}