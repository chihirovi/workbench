#include "RasterPipelineTemplate.hpp"

namespace Irori::Core{

// begin renderpass -> bind pipeline -> bind descsets の順が推奨らしい
 UniqueRasterPipelineTemplate& UniqueRasterPipelineTemplate::cmd_begin_renderpass(vk::CommandBuffer& cb){
    vk::RenderPassBeginInfo begin_info;
    begin_info
        .setRenderPass(*renderpass)
        .setFramebuffer(*framebuffer)
        .setRenderArea(vk::Rect2D({0,0}, {width, height}));

    cb.beginRenderPass(begin_info, vk::SubpassContents::eInline);
    return *this;
}

UniqueRasterPipelineTemplate& UniqueRasterPipelineTemplate::cmd_clear_attachments(vk::CommandBuffer& cb){
    cb.clearAttachments(clear_attachments, clear_rects);
    return *this;
}

UniqueRasterPipelineTemplate& UniqueRasterPipelineTemplate::cmd_bind_pipeline(vk::CommandBuffer& cb){
    cb.bindPipeline(vk::PipelineBindPoint::eGraphics, *pipeline);
    return *this;
}

void UniqueRasterPipelineTemplate::cmd_bind_local_descsets(vk::CommandBuffer& cb){
    descriptor_sets.bind_local_descriptor_sets(
        cb,
        *pipeline_layout,
        vk::PipelineBindPoint::eGraphics
    );   
}

//builder methods
UniqueRasterPipelineTemplate::Builder& UniqueRasterPipelineTemplate::Builder::add_color_attachment_info(
    ColorAttachmentInfo color_attachment_info){

    color_attachment_names.push_back(color_attachment_info.name);
    color_attachment_images.push_back(color_attachment_info.image);
    color_attachment_blend_states.push_back(color_attachment_info.blend_state);
    if(color_attachment_info.clear_rgba){
        auto rgba = color_attachment_info.clear_rgba.value();
        vk::ClearColorValue ccv = vk::ClearColorValue{rgba[0], rgba[1], rgba[2], rgba[3]};
        color_attachment_clear_values.push_back(ccv);
    }else{
        color_attachment_clear_values.push_back(std::nullopt);
    }
    
    return *this;
}

UniqueRasterPipelineTemplate UniqueRasterPipelineTemplate::Builder::build(){

    // まずはシェーダーの構成を確かめる
    enum ShaderLocation{
        VERTEX      = 0,
        TASK        = 1,
        MESH        = 2,
        FRAGMENT    = 3,
    };
    int shader_location_map[4] = {-1,-1,-1,-1}; //vertex, task, mesh, fragment
                                                //
    auto find_shader_location = [&](vk::ShaderStageFlagBits stage, ShaderLocation location){
        for(int i=0; i<pipeline_shader_resource_info->shader_infos.size(); i++){
            if(pipeline_shader_resource_info->shader_infos[i].stage_flag == stage){
                shader_location_map[location] = i;
                break;
            }
        }
    };
    
    find_shader_location(vk::ShaderStageFlagBits::eVertex, ShaderLocation::VERTEX);
    find_shader_location(vk::ShaderStageFlagBits::eTaskEXT, ShaderLocation::TASK);
    find_shader_location(vk::ShaderStageFlagBits::eMeshEXT, ShaderLocation::MESH);
    find_shader_location(vk::ShaderStageFlagBits::eFragment, ShaderLocation::FRAGMENT);

    bool is_vertex_frag_pipeline = false;
    bool is_task_mesh_frag_pipeline = false;
    bool is_mesh_frag_pipeline = false;
    
    uint32_t shader_num = pipeline_shader_resource_info->shader_infos.size();
    int vertex_loc = shader_location_map[ShaderLocation::VERTEX];
    int task_loc = shader_location_map[ShaderLocation::TASK];
    int mesh_loc = shader_location_map[ShaderLocation::MESH];
    int frag_loc = shader_location_map[ShaderLocation::FRAGMENT];

    if(shader_num==2 && vertex_loc >= 0 && frag_loc >= 0){
        is_vertex_frag_pipeline = true; 

    }else if(shader_num==3 && task_loc >= 0 && mesh_loc >= 0 && frag_loc >= 0){
        is_task_mesh_frag_pipeline = true; 

    }else if(shader_num==2 && mesh_loc >= 0 && frag_loc >= 0){
        is_mesh_frag_pipeline = true; 

    }else{
        printf("------------\n");
        for(auto& t : pipeline_shader_resource_info->shader_infos){
            printf("%s, ", vk::to_string(t.stage_flag).c_str());
        }
        printf("\n");
        ASSERT_WITH_MSG(false, "This shader setup does not satisfy the conditions of the Irori rasterization pipeline");
    }
    
    // その他の設定が正しいか確かめる
    uint32_t frag_color_attachment_num = pipeline_shader_resource_info->shader_infos[frag_loc].color_attachment_location_map.size();
    ASSERT_WITH_MSG((color_attachment_names.size() == frag_color_attachment_num), std::format("The number of color attachments must match. {}, {}", color_attachment_names.size(), frag_color_attachment_num));
    ASSERT_WITH_MSG((w != 0), "width can't be 0");
    ASSERT_WITH_MSG((h != 0), "height can't be 0");
    
    // attachment 類を並べ替える (別にやる必要はないといえばないが，やっておいたほうが楽)
    auto sort_color_attachment_infos = [&]<typename T>(std::vector<T>* infos){
        std::vector<T> temp(infos->size());
        std::copy(infos->begin(), infos->end(), temp.begin());
        for(int i=0; i<infos->size(); i++){
            uint32_t attachment_location = pipeline_shader_resource_info->shader_infos[frag_loc].get_color_attachment_location(color_attachment_names[i]);
            (*infos)[attachment_location] = temp[i];
        }
    };
    sort_color_attachment_infos(&color_attachment_images);
    sort_color_attachment_infos(&color_attachment_blend_states);
    sort_color_attachment_infos(&color_attachment_clear_values);
    
    // ------------------------------------------------------
    // creation start
    UniqueRasterPipelineTemplate raster_pipeline_template(vc);
    raster_pipeline_template.width = w;
    raster_pipeline_template.height = h;
    
    // create desc set
    raster_pipeline_template.descriptor_sets = UniqueDescriptorSets::Local(vc, pipeline_shader_resource_info->local_descsets_info, *global_descsets_info);
    
    // create shader modules
    raster_pipeline_template.shader_modules = UniqueShaderModules(vc, pipeline_shader_resource_info->shader_infos);
    
    // color create attachments
    std::vector<vk::AttachmentDescription> color_depth_attachments; 
    color_depth_attachments.resize(frag_color_attachment_num);

    for(int i = 0; i<frag_color_attachment_num; i++){

        // clearはclearFrameBufferコマンドで別途で行うので，ロードオプションはload固定でいい
        color_depth_attachments[i]
            .setFormat(color_attachment_images[i]->image_create_info.format)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(vk::ImageLayout::eColorAttachmentOptimal)
            .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal);
    }

    // depth attachmentがあるなら，最後に入れる.
    if(depth_test_enable){

        ASSERT_WITH_MSG((depth_attachment_image != nullptr), "when depth test enabled, depth attachment image must be valid handle");
        
        color_depth_attachments.push_back(vk::AttachmentDescription());

        if(depth_write_enable){
            color_depth_attachments.back()
                .setFormat(depth_attachment_image->image_create_info.format)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eStore)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        }else{
            color_depth_attachments.back()
                .setFormat(depth_attachment_image->image_create_info.format)
                .setSamples(vk::SampleCountFlagBits::e1)
                .setLoadOp(vk::AttachmentLoadOp::eLoad)
                .setStoreOp(vk::AttachmentStoreOp::eDontCare)
                .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
                .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
                //.setInitialLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal)
                //.setFinalLayout(vk::ImageLayout::eDepthReadOnlyStencilAttachmentOptimal)
                .setInitialLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
                .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal);
        }
    }
    
    // create attachment refs
    std::vector<vk::AttachmentReference> color_attachment_refs;
    color_attachment_refs.resize(frag_color_attachment_num);

    for(int i = 0; i<frag_color_attachment_num; i++){

        color_attachment_refs[i]
            .setAttachment(i)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
    }
    
    vk::AttachmentReference depth_attachment_ref;
    if(depth_test_enable){
        depth_attachment_ref
            .setAttachment(color_depth_attachments.size()-1)
            .setLayout(color_depth_attachments.back().initialLayout);
    }

    // create renderpass
    vk::SubpassDescription subpasses[1];
    subpasses[0]
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        .setColorAttachments(color_attachment_refs);

    if(depth_test_enable){
        subpasses[0]
            .setPDepthStencilAttachment(&depth_attachment_ref);
    }
    
    vk::RenderPassCreateInfo renderpass_create_info;
    renderpass_create_info
        .setAttachments(color_depth_attachments)
        .setSubpasses(subpasses)
        .setDependencies({});
    
    raster_pipeline_template.renderpass = raster_pipeline_template.vulkan_context->device->createRenderPassUnique(renderpass_create_info); 
    
    // create frame buffer
    std::vector<vk::ImageView> image_views;
    image_views.resize(frag_color_attachment_num);
    
    for(int i=0; i<frag_color_attachment_num; i++){
        image_views[i] = color_attachment_images[i]->image_view.get();
    }

    if(depth_test_enable){
        image_views.push_back(depth_attachment_image->image_view.get());
    }

    vk::FramebufferCreateInfo framebuf_create_info;
    framebuf_create_info
        .setWidth(raster_pipeline_template.width)
        .setHeight(raster_pipeline_template.height)
        .setLayers(1)
        .setRenderPass(*raster_pipeline_template.renderpass)
        .setAttachments(image_views);
    
    raster_pipeline_template.framebuffer = raster_pipeline_template.vulkan_context->device->createFramebufferUnique(
        framebuf_create_info
    );  
    
    // create pipeline layout
    auto temp = raster_pipeline_template.descriptor_sets.get_descset_layouts();
    vk::PipelineLayoutCreateInfo layout_create_info;
    layout_create_info
        .setSetLayouts(temp);
    
    raster_pipeline_template.pipeline_layout = raster_pipeline_template.vulkan_context->device->createPipelineLayoutUnique(
        layout_create_info
    );

    // create pipeline
    vk::Viewport viewports[1];
    viewports[0]
        .setX(0.0)
        .setY(0.0)
        .setMinDepth(0.0)
        .setMaxDepth(1.0)
        .setWidth(raster_pipeline_template.width)
        .setHeight(raster_pipeline_template.height);
    
    vk::Rect2D scissors[1];
    scissors[0]
        .setOffset({0, 0})
        .setExtent({raster_pipeline_template.width, raster_pipeline_template.height});
    
    vk::PipelineViewportStateCreateInfo viewport_state;
    viewport_state
        .setViewports(viewports)
        .setScissors(scissors);

    vk::PipelineVertexInputStateCreateInfo vertex_input_info;
    if(vertex_input_descs!=nullptr && input_attrib_descs!=nullptr){
        vertex_input_info
            .setVertexBindingDescriptions(*vertex_input_descs)
            .setVertexAttributeDescriptions(*input_attrib_descs);
    }
    
    vk::PipelineInputAssemblyStateCreateInfo input_assembly;
    input_assembly
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        .setPrimitiveRestartEnable(vk::False);
    
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer
        .setDepthClampEnable(vk::False)
        .setRasterizerDiscardEnable(vk::False)
        .setPolygonMode(polygon_mode)
        .setLineWidth(1.0f)
        .setCullMode(culling_mode)
        .setFrontFace(front_face)
        .setDepthBiasEnable(vk::False);

    vk::PipelineMultisampleStateCreateInfo multi_sample;
    multi_sample
        .setSampleShadingEnable(vk::False)
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);
    
    std::vector<vk::PipelineColorBlendAttachmentState> blend_attachments;
    blend_attachments.resize(frag_color_attachment_num);

    for(int i=0; i<frag_color_attachment_num; i++){

        if(color_attachment_blend_states[i]){
            blend_attachments[i] = color_attachment_blend_states[i].value();

        }else{
            blend_attachments[i]
                .setColorWriteMask( //ここを設定するの超大事，blendするかどうかと関係なく，書き込みのbitを制御してる
                    vk::ColorComponentFlagBits::eR|
                    vk::ColorComponentFlagBits::eG|
                    vk::ColorComponentFlagBits::eB|
                    vk::ColorComponentFlagBits::eA
                )
                .setBlendEnable(vk::False);
        }
    }

    vk::PipelineColorBlendStateCreateInfo blend;
    blend.setLogicOpEnable(vk::False)
        .setAttachments(blend_attachments);
    
    vk::PipelineDepthStencilStateCreateInfo depth_stencil;
    depth_stencil
        .setDepthTestEnable(depth_test_enable)
        .setDepthWriteEnable(depth_write_enable)
        .setDepthCompareOp(depth_compare_op)
        .setDepthBoundsTestEnable(vk::False)
        .setStencilTestEnable(vk::False);
    
    vk::GraphicsPipelineCreateInfo pipeline_create_info;
    pipeline_create_info
        .setPViewportState(&viewport_state)
        .setPVertexInputState(&vertex_input_info)
        .setPInputAssemblyState(&input_assembly)
        .setPRasterizationState(&rasterizer)
        .setPMultisampleState(&multi_sample)
        .setPColorBlendState(&blend)
        .setLayout(*raster_pipeline_template.pipeline_layout)
        .setStages(raster_pipeline_template.shader_modules.pipeline_shader_create_infos) //max 5 stages , vert, tess_ctrl, tess_evel, geo, frag
        .setRenderPass(*raster_pipeline_template.renderpass)
        .setPDepthStencilState(&depth_stencil)
        .setSubpass(0);

    auto result = raster_pipeline_template.vulkan_context->device->createGraphicsPipelineUnique(
        nullptr,
        pipeline_create_info
    );
    
    VKHPP_CHECK(result);

    raster_pipeline_template.pipeline = std::move(result.value);
    
    // create clear attachments
    raster_pipeline_template.clear_attachments.resize(frag_color_attachment_num);
    raster_pipeline_template.clear_rects.resize(frag_color_attachment_num);

    for(int i=0; i<frag_color_attachment_num; i++){

        if(color_attachment_clear_values[i]){
            raster_pipeline_template.clear_attachments[i]
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setColorAttachment(i)
                .setClearValue(color_attachment_clear_values[i].value());

            raster_pipeline_template.clear_rects[i]
                .setRect(vk::Rect2D{{0,0},{raster_pipeline_template.width, raster_pipeline_template.height}})
                .setBaseArrayLayer(0)
                .setLayerCount(1);
        }
    }

    if(clear_depth_value){
        ASSERT_WITH_MSG((depth_test_enable), "can't set [clear depth enable = true], when depth test is disable");

        raster_pipeline_template.clear_attachments.push_back(vk::ClearAttachment());
        raster_pipeline_template.clear_rects.push_back(vk::ClearRect());

        auto cds = vk::ClearDepthStencilValue();
        cds .setDepth(clear_depth_value.value())
            .setStencil(0);
        auto cv = vk::ClearValue();
        cv.setDepthStencil(cds);

        raster_pipeline_template.clear_attachments.back()
            .setAspectMask(vk::ImageAspectFlagBits::eDepth)
            .setClearValue(cv);

        raster_pipeline_template.clear_rects.back()
            .setRect(vk::Rect2D{{0,0},{raster_pipeline_template.width, raster_pipeline_template.height}})
            .setBaseArrayLayer(0)
            .setLayerCount(1);
    }
    
    return std::move(raster_pipeline_template);
}

}