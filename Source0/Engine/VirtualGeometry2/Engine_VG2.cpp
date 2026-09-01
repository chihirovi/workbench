#include "Engine_VG2.hpp"
#include "../../VirtualGeometry/VG2.hpp"
#include<algorithm>

namespace Irori::Engine::VirtualGeometry2{

UniqueEngine::UniqueEngine(std::string config_file){
    init_base(config_file);
    init_buffers();
    init_images();
    init_pipelines();
}

void UniqueEngine::init_base(std::string& config_file){
    
    // config file
    INIReader reader(config_file);

    if (reader.ParseError() < 0) {
        std::cerr << "Can't load " << config_file << "\n";
        exit(1);
    }

    window_width  = reader.GetInteger("General", "WindowWidth", 1920);
    window_height = reader.GetInteger("General", "WindowHeight", 1200);
    render_target_width = window_width;
    render_target_height = window_height;

    // window
    window = UniqueVkGlfwWindow(window_width, window_height, "IroriEngine Version 0.1.0"); //init fileから読むようにする
    window.set_fps_limit(60.0);
    window.set_cursor_visible();
    //window.set_fps_limit(1000.0);
                                                                    
    // vulkan context
    Core::configure_vulkan_context_with_window(&vulkan_context, &window);
    vulkan_context.init();
    
    // swapchain
    swapchain = Core::UniqueSwapchain(&vulkan_context);
    
    // sampler
    sampler_pool = Core::UniqueSamplerPool(&vulkan_context);

    // scene manager
    scene_manager = Scene::UniqueSceneManager(&vulkan_context);
    
    // editor
    editor = UniqueEditor(&vulkan_context, swapchain);
    
    ////////////////////////////////
    ////////////////////////////////
    ////////////////////////////////
    // test scene //あとで何とかする
    float d_light_power = 100.0;
    scene_manager
        .set_directional_light({1.0, 1.0, 1.0, d_light_power}, {0.4, 1.1, 0.4}) //垂直放射照度
        .set_sky_light({1.0, 1.0, 1.0, d_light_power/(2.0*3.1415)});

    //std::string vg_path = "./Assets/bunny_tex/bunny_tex.vg";
    std::string vg_path = "./Tools/VGGenerator2/Log/mesh0.vg2";
    scene_manager
        //.register_model("bunny", "./Assets/bunny_tex/scene.gltf", vg_path);
        .register_model("bunny", "./Assets/Bunny/bunny.gltf", vg_path);
        ;
        
    // load vg2 file
    {   
        auto vg_file = Irori::VirtualGeometry2::load_vg2_file(vg_path.c_str());

        uint32_t size = sizeof(Irori::VirtualGeometry2::ClusterVG2) * vg_file.header.cluster_count;
        auto c_buf = Core::UniqueBuffer::common(&vulkan_context, size, false, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress|vk::BufferUsageFlagBits::eTransferDst, 8);
        c_buf.transfer_data(vg_file.clusters.data(), size);

        size = sizeof(float) * 3 * vg_file.header.vertex_count;
        auto v_buf = Core::UniqueBuffer::common(&vulkan_context, size, false, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress|vk::BufferUsageFlagBits::eTransferDst, 8);
        v_buf.transfer_data(vg_file.vertex_positions.data(), size);
        
        size = sizeof(uint8_t) * vg_file.header.index_count;
        auto i_buf = Core::UniqueBuffer::common(&vulkan_context, size, false, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress|vk::BufferUsageFlagBits::eTransferDst, 8);
        i_buf.transfer_data(vg_file.indices.data(), size);
        
        size = sizeof(Irori::VirtualGeometry2::BVH8NodeVG2) * vg_file.header.bvh8_node_count;
        auto n_buf = Core::UniqueBuffer::common(&vulkan_context, size, false, vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress|vk::BufferUsageFlagBits::eTransferDst, 8);
        n_buf.transfer_data(vg_file.bvh8_nodes.data(), size);

        scene_manager.model_manager.models_data["bunny"].instance_descs[0].cluster_buffer_idx = 0;
        scene_manager.model_manager.models_data["bunny"].instance_descs[0].cluster_vertex_buffer_idx = 0;
        scene_manager.model_manager.models_data["bunny"].instance_descs[0].cluster_index_buffer_idx = 0;
        scene_manager.model_manager.models_data["bunny"].instance_descs[0].cluster_bvh8_buffer_idx = 0;
        scene_manager.model_manager.models_data["bunny"].instance_descs[0].cluster_num = vg_file.header.cluster_count;
        scene_manager.model_manager.models_data["bunny"].cluster_buffers.push_back(std::move(c_buf));
        scene_manager.model_manager.models_data["bunny"].cluster_vertex_buffers.push_back(std::move(v_buf));
        scene_manager.model_manager.models_data["bunny"].cluster_index_buffers.push_back(std::move(i_buf));
        scene_manager.model_manager.models_data["bunny"].cluster_bvh_buffers.push_back(std::move(n_buf));
    }

    // todo 
    // 1. あとからClusterのデータ代入をできるようなAPIを追加する
    // 2. Vertex の種類を指定できるようにする
    // 3. Vertex なしでも行けるようにする（cluster のみ）
    /*
    for(int x = 0; x < 10; x++){
        for(int z = 0; z < 50; z++){
            std::string name = std::format("bunny_{}_{}_{}", x, 0, z);
            scene_manager
                .instantiate(name, "bunny")
                .apply_transform(
                    name, 
                    glm::translate(glm::f32mat4x4(1.0), 
                        {
                            x*1.0f- 10*1.0f/2.0f,
                            z*0.4,
                            z*-1.0f
                        }
                    )
                    *glm::scale(glm::f32mat4x4(1.0), {4, 4, 4})
                );
        }
    }
    */
    scene_manager
        .instantiate("bunny0", "bunny")
        //.apply_transform("bunny0", glm::translate(glm::scale(glm::f32mat4x4(1.0), {0.01, 0.01, 0.01}), {0.0f, 0.0f, 0.0f}))
        .apply_transform("bunny0", glm::translate(glm::scale(glm::f32mat4x4(1.0), {4, 4, 4}), {0.0f, 0.0f, 0.0f}))
        //.instantiate("bunny1", "bunny")
        //.apply_transform("bunny1", glm::translate(glm::scale(glm::f32mat4x4(1.0), {4, 4, 4}), {0.1f, 0.0f, 0.0f}))
        ;

    glm::f32vec3 look_from = {0.0f, 0.5f, 3.0f};
    glm::f32vec3 look_at_d = {0.0f, 0.0f, -1.0f};
    look_at_d = glm::normalize(look_at_d);
    scene_manager
        .init_camera(render_target_width, render_target_height, look_from, look_from + look_at_d, 90.0f, {0,1,0});
    
    // VG
    tile_num_x = (render_target_width  + CLASSIFY_TILE_WIDTH -1)/CLASSIFY_TILE_WIDTH;
    tile_num_y = (render_target_height + CLASSIFY_TILE_WIDTH -1)/ CLASSIFY_TILE_WIDTH;
    tile_num = tile_num_x * tile_num_y;
}


void UniqueEngine::run(){
    
    uint32_t frame_counter = 0;

    

    // frame rendrer
    auto frame_renderer = [&](vk::CommandBuffer& command_buffer, Core::UniqueImage& final_dst_image){

        if(frame_counter == 0) return;
        
        // 本当はここでこんなことするべきじゃないが
        uint32_t total_cluster_num = 0;
        if(frame_counter > 0){
            std::vector<uint32_t> cluster_num_prefixsum;
            cluster_num_prefixsum.push_back(0);

            for(auto& v_ref : cpu_frame_render_data.virtual_geometry_opaque_instance_refs){
                auto& inst = cpu_frame_render_data.render_instance_table[v_ref.render_instance_id_base + v_ref.render_instance_id];
                cluster_num_prefixsum.push_back(inst.cluster_num);
            }
            
            for(int i=1; i<cluster_num_prefixsum.size(); i++){
                cluster_num_prefixsum[i] += cluster_num_prefixsum[i-1];
            }
            total_cluster_num = cluster_num_prefixsum.back();
            cluster_num_prefixsum.push_back(0);
            
            buffer_registry["cluster_num_prefixsum"].transfer_data(cluster_num_prefixsum.data(), cluster_num_prefixsum.size()*sizeof(uint32_t));
        }

        //////////////////////////////
        // update global descriptor sets
        global_descriptor_sets
            .update("textures", cpu_frame_render_data.texture_table, vk::ImageLayout::eShaderReadOnlyOptimal);
        
        //////////////////////////////
        // transfer data
        // gpu frame render data
        buffer_registry["gpu_frame_render_data_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["gpu_frame_render_data"], 
            sizeof(Render::GPU_FrameRenderData)
        );

        // instance table
        buffer_registry["instance_table_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["instance_table"], 
            cpu_frame_render_data.render_instance_table.size()*sizeof(Render::Instance)
        );

        // instance reference
        buffer_registry["instance_reference_table_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["instance_reference_table"], 
            current_frame_render_instance_reference_count*sizeof(Render::InstanceReference)
        );

        // material table
        buffer_registry["material_table_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["material_table"], 
            cpu_frame_render_data.material_table.size()*sizeof(Render::Material)
        );
        
        // texture view table
        buffer_registry["texture_view_table_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["texture_view_table"], 
            cpu_frame_render_data.texture_view_table.size()*sizeof(Render::TextureView)
        );
        
        //////////////////////////////
        // buffer barrier
        Core::CmdBufferBarrier::transfer_dst_2_comp_read(command_buffer, buffer_registry["gpu_frame_render_data"]);
        Core::CmdBufferBarrier::transfer_dst_2_comp_read(command_buffer, buffer_registry["instance_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_comp_read(command_buffer, buffer_registry["instance_reference_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_comp_read(command_buffer, buffer_registry["material_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_comp_read(command_buffer, buffer_registry["texture_view_table"]);
        
        




        
        //////////////////////////////
        // main pass
        
        // clear
        clear_vg_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        Core::CmdBufferBarrier::comp_write_2_comp_read(command_buffer, buffer_registry["cluster_render_queue"]);
        Core::CmdImageBarrier::comp_write_2_frag_read_G2G(command_buffer, image_registry["visiblity"]);
        Core::CmdImageBarrier::comp_write_2_comp_read_G2G(command_buffer, image_registry["mat_tile_output"]);
        Core::CmdBufferBarrier::comp_write_2_comp_read(command_buffer, buffer_registry["tile_indirect_draw_arg"]);
        
        // cluster select
        cluster_select_pipeline.cmd_dispatch(command_buffer,  total_cluster_num, global_descriptor_sets);
        Core::CmdBufferBarrier::comp_write_2_comp_read(command_buffer, buffer_registry["cluster_render_queue"]);
        
        // mesh indirect arg

        // cluster  mesh
        cluster_mesh_pipeline.cmd_dispatch(command_buffer, buffer_registry["mesh_draw_arg"], 
            Irori::VirtualGeometry::UniqueClusterMeshPipeline::Stage::OC_1,
            global_descriptor_sets);
        Core::CmdImageBarrier::comp_write_2_comp_read_G2G(command_buffer, image_registry["visiblity"]);
        
        // material
        material_id_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        Core::CmdImageBarrier::comp_write_2_comp_read_G2G(command_buffer, image_registry["material_id"]);
        Core::CmdImageBarrier::comp_write_2_frag_read_G2G(command_buffer, image_registry["material_id"]);
        
        // materila classify
        material_classify_pipeline.cmd_dispatch(command_buffer, tile_num_x, tile_num_y, global_descriptor_sets);
        Core::CmdBufferBarrier::comp_write_2_indirect_arg(command_buffer, buffer_registry["tile_indirect_draw_arg"]);
        Core::CmdBufferBarrier::comp_write_2_vert_read(command_buffer, buffer_registry["tile_index"]);

        // material id 2 depth
        matid2depth_pipeline.cmd_dispatch(command_buffer, global_descriptor_sets);
        Core::CmdImageBarrier::frag_CA_write_2_frag_CA_read_D2D(command_buffer, image_registry["material_id_depth"]);
        
        // material tile
        material_tile_pipeline.cmd_dispatch(command_buffer, buffer_registry["tile_indirect_draw_arg"], global_descriptor_sets);
        Core::CmdImageBarrier::frag_write_2_comp_read_G2G(command_buffer, image_registry["mat_tile_output"]);
        
        // vg debug
        vg_debug_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        





        
        //////////////////////////////
        // image barrier
        Core::CmdImageBarrier::comp_write_2_transfer_G2Tsrc(command_buffer, image_registry["output"]);
        
        // imgui pass
        Core::CmdImageBarrier::top_pipe_2_transfer_CAopt2Tdst(command_buffer, editor.render_image);
        image_registry["output"].cmd_copy_to_image(command_buffer, editor.render_image);
        Core::CmdImageBarrier::transfer_2_frag_write_CA_Tdst2CAopt(command_buffer, editor.render_image);
        editor.cmd_render(command_buffer, scene_manager);

        // copy image pass
        Core::CmdImageBarrier::top_pipe_2_transfer_Psrc2Tdst(command_buffer, final_dst_image);
        Core::CmdImageBarrier::frag_CA_write_2_transfer_CAopt2Tsrc(command_buffer, editor.render_image);
        editor.render_image.cmd_copy_to_image(command_buffer, final_dst_image);
        

        // イメージのレイアウトを元に戻す
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tsrc2G(command_buffer, image_registry["output"]);
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tdst2Psrc(command_buffer, final_dst_image);
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tdst2fragCA(command_buffer, editor.render_image);
    };

    // cpu task to update resource
    auto cpu_async_task_in_rendering = [&](){
        // imgui upate prosess
        bool key_response[7] = {false, false, false, false, false, false, false};
        float mouse_offset[2] = {0.0f, 0.0f};
        // update camera
        if(editor.camera_lock == false){
            for(int i=0; i<7; i++) key_response[i] = window.get_key_response()[i];
        }
        if(editor.camera_lock == false && 
            window.is_cursor_in_window() && 
            window.is_mouse_left_botton_pressed()){
            for(int i=0; i<2; i++) mouse_offset[i] = window.get_mouse_offset()[i] * 3.0;
        }
        if(editor.is_using_gui == false){
            scene_manager.update_camera(window.time_delta(), key_response, mouse_offset);
        }
        if(editor.camera_reset) scene_manager.camera.reset();
        
        cpu_frame_render_data.editor_data[0] = editor.selected_mode;
        cpu_frame_render_data.editor_data[1] = editor.selected_lod;
        cpu_frame_render_data.editor_data[2] = editor.is_auto ? 1 : 0;

        // build cpu frame render data
        cpu_frame_render_data.reset();

        scene_manager.build_cpu_frame_render_data(&cpu_frame_render_data);

        cpu_frame_render_data.render_target_width = render_target_width;
        cpu_frame_render_data.render_target_height = render_target_height;
        
        // transfer data to staging buffers
        cpu_frame_render_data_to_staging_buffers();
    };

    auto cpu_task_after_rendering = [&](float frame_time_ms){
        //editor
        editor.frame_time_ms = frame_time_ms;

        frame_counter++;
        
        if(frame_counter % 60 == 0){
            //if(frame_counter == 60) scene_manager.camera.reset();
        }
    };

    // begin render loop
    Render::render_loop(
        vulkan_context, 
        swapchain, 
        [&](){},
        frame_renderer, 
        cpu_async_task_in_rendering,
        cpu_task_after_rendering
    );
}


void UniqueEngine::init_buffers(){
    // create buffers
    // gpu frame rende data
    buffer_registry.reg_resource("gpu_frame_render_data");
    buffer_registry["gpu_frame_render_data"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(Render::GPU_FrameRenderData),
        false,
        vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("gpu_frame_render_data_staging");
    buffer_registry["gpu_frame_render_data_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(Render::GPU_FrameRenderData)
    );
    
    // instance table
    buffer_registry.reg_resource("instance_table");
    buffer_registry["instance_table"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(Render::Instance)*MAX_RENDER_INSTANCE_NUM,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("instance_table_staging");
    buffer_registry["instance_table_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(Render::Instance)*MAX_RENDER_INSTANCE_NUM
    );

    // instance reference table
    buffer_registry.reg_resource("instance_reference_table");
    buffer_registry["instance_reference_table"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(Render::InstanceReference)*MAX_RENDER_INSTANCE_NUM,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("instance_reference_table_staging");
    buffer_registry["instance_reference_table_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(Render::InstanceReference)*MAX_RENDER_INSTANCE_NUM
    );

    // material table
    buffer_registry.reg_resource("material_table");
    buffer_registry["material_table"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(Render::Material)*MAX_MATERIAL_NUM,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("material_table_staging");
    buffer_registry["material_table_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(Render::Material)*MAX_MATERIAL_NUM
    );   

    // texture view table    
    buffer_registry.reg_resource("texture_view_table");
    buffer_registry["texture_view_table"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(Render::TextureView)*MAX_TEXTURE_VIEW_NUM,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("texture_view_table_staging");
    buffer_registry["texture_view_table_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(Render::TextureView)*MAX_TEXTURE_VIEW_NUM
    );

    ////////////////////////
    // VG
    buffer_registry.reg_resource("cluster_num_prefixsum");
    buffer_registry["cluster_num_prefixsum"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*1024*8,
        true,
        vk::BufferUsageFlagBits::eStorageBuffer
    );

    buffer_registry.reg_resource("cluster_render_queue");
    buffer_registry["cluster_render_queue"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*2 + sizeof(uint64_t)*65536*8,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer
    );
    
    buffer_registry.reg_resource("mesh_draw_arg");
    buffer_registry["mesh_draw_arg"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*3*1024,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer
    );

    buffer_registry.reg_resource("tile_indirect_draw_arg");
    buffer_registry["tile_indirect_draw_arg"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*4*MAX_MATERIAL,
        true,
        vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eIndirectBuffer
    );

    buffer_registry.reg_resource("tile_index");
    buffer_registry["tile_index"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*tile_num*MAX_MATERIAL,
        true,
        vk::BufferUsageFlagBits::eStorageBuffer
    );

    buffer_registry.reg_resource("debug_buffer");
    buffer_registry["debug_buffer"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint32_t)*tile_num*MAX_MATERIAL,
        true,
        vk::BufferUsageFlagBits::eStorageBuffer
    );
}


void UniqueEngine::init_images(){
    // output image
    image_registry.reg_resource("output");
    image_registry["output"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eGeneral
        )
        .build();
    
    // VG
    image_registry.reg_resource("visiblity");
    image_registry["visiblity"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR64Uint,
            vk::ImageUsageFlagBits::eStorage,
            vk::ImageLayout::eGeneral
        )
        .build();
    
    // material id
    image_registry.reg_resource("material_id");
    image_registry["material_id"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR32Uint,
            vk::ImageUsageFlagBits::eStorage,
            vk::ImageLayout::eGeneral
        )
        .build();
    
    // material id depth
    image_registry.reg_resource("material_id_depth");
    image_registry["material_id_depth"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
        .build();

    // material tile output
    image_registry.reg_resource("mat_tile_output");
    image_registry["mat_tile_output"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eStorage,
            vk::ImageLayout::eGeneral
        )
        .build();
}


void UniqueEngine::init_pipelines(){
    //create pipeline registry
    Irori::Core::ShaderReflect::PipelineRegistry pipeline_registry;
    
    // clear vg
    pipeline_registry.register_pipeline_name("clear_vg");
    pipeline_registry["clear_vg"]
    .add_shader_info(
        "./Shaders/Spv/ClearVG2.spv",
        "main_comp"
    );     

    // cluster select
    pipeline_registry.register_pipeline_name("cluster_select");
    pipeline_registry["cluster_select"]
    .add_shader_info(
        "./Shaders/Spv/SelectCluster2.spv",
        "main_comp"
    );  
    
    // cluster select
    pipeline_registry.register_pipeline_name("mesh_arg");
    pipeline_registry["mesh_arg"]
    .add_shader_info(
        "./Shaders/Spv/MeshIndirectArg2.spv",
        "main_comp"
    );  

    // cluster select
    pipeline_registry.register_pipeline_name("vg_debug");
    pipeline_registry["vg_debug"]
    .add_shader_info(
        "./Shaders/Spv/VGDebug2.spv",
        "main_comp"
    );  

    // cluster select
    pipeline_registry.register_pipeline_name("material_id");
    pipeline_registry["material_id"]
    .add_shader_info(
        "./Shaders/Spv/WriteMatID2.spv",
        "main_comp"
    );  

    // material classify
    pipeline_registry.register_pipeline_name("material_classify");
    pipeline_registry["material_classify"]
    .add_shader_info(
        "./Shaders/Spv/MaterialClassify2.spv",
        "main_comp"
    );  

    // cluster mesh
    pipeline_registry.register_pipeline_name("cluster_mesh");
    pipeline_registry["cluster_mesh"]
    .add_shader_info(
        "./Shaders/Spv/ClusterMesh2.spv",
        "main_mesh"
    ).add_shader_info(
        "./Shaders/Spv/ClusterFrag2.spv",
        "main_fragment"
    );  
 
    // mat id 2 depth
    pipeline_registry.register_pipeline_name("mat_id_2_depth");
    pipeline_registry["mat_id_2_depth"]
    .add_shader_info(
        "./Shaders/Spv/MatID2Depth2.spv",
        "main_vert"
    ).add_shader_info(
        "./Shaders/Spv/MatID2Depth2.spv",
        "main_fragment"
    );     
    
    // mat id 2 depth
    pipeline_registry.register_pipeline_name("material_tile");
    pipeline_registry["material_tile"]
    .add_shader_info(
        "./Shaders/Spv/MaterialTile2.spv",
        "main_vert"
    ).add_shader_info(
        "./Shaders/Spv/MaterialTile2.spv",
        "main_fragment"
    );     

    // build global descriptor set
    auto global_descset_build_info = Irori::Core::ShaderReflect::extract_global_and_build_local_descsets_info(pipeline_registry);

    global_descriptor_sets = Irori::Core::UniqueDescriptorSets::Global(
        &vulkan_context,
        global_descset_build_info
    );
    
    // VG
    global_descriptor_sets
        .update("cluster_num_prefixsum", buffer_registry["cluster_num_prefixsum"])
        .update("cluster_render_queue", buffer_registry["cluster_render_queue"])
        .update("visiblity_buffer", image_registry["visiblity"], vk::ImageLayout::eGeneral)
        .update("mesh_draw_arg", buffer_registry["mesh_draw_arg"])
        .update("output", image_registry["output"], vk::ImageLayout::eGeneral)
        .update("material_id", image_registry["material_id"], vk::ImageLayout::eGeneral)
        .update("tile_indirect_draw_arg", buffer_registry["tile_indirect_draw_arg"])
        .update("tile_index", buffer_registry["tile_index"])
        .update("mat_tile_output", image_registry["mat_tile_output"], vk::ImageLayout::eGeneral)
        //.update("debug_buffer", buffer_registry["debug_buffer"])
    ;

    //////////////////////
    // bind resource to global descriptor set
    auto samplers = sampler_pool.get_samplers();
    global_descriptor_sets
        .update("frame_render_data", buffer_registry["gpu_frame_render_data"])
        .update("samplers", samplers);
        ;
    
    ///////////////////
    // build pipelines
    clear_vg_pipeline = Irori::VirtualGeometry::UniqueClearVGPipeline(
        &vulkan_context,
        pipeline_registry["clear_vg"],
        global_descset_build_info
    );

    cluster_select_pipeline = Irori::VirtualGeometry::UniqueClusterSelectPipeline(
        &vulkan_context,
        pipeline_registry["cluster_select"],
        global_descset_build_info
    );

    cluster_mesh_pipeline = Irori::VirtualGeometry::UniqueClusterMeshPipeline(
        &vulkan_context,
        render_target_width, render_target_height,
        pipeline_registry["cluster_mesh"],
        global_descset_build_info
    );
    
    vg_debug_pipeline = Irori::VirtualGeometry::UniqueVGDebugPipeline(
        &vulkan_context,
        pipeline_registry["vg_debug"],
        global_descset_build_info
    );

    material_id_pipeline = Irori::VirtualGeometry::UniqueVGDebugPipeline(
        &vulkan_context,
        pipeline_registry["material_id"],
        global_descset_build_info
    );

    material_classify_pipeline = Irori::VirtualGeometry::UniqueMaterialClassifyPipeline(
        &vulkan_context,
        pipeline_registry["material_classify"],
        global_descset_build_info
    );
    
    matid2depth_pipeline = Irori::VirtualGeometry::UniqueMatID2DepthPipeline(
        &vulkan_context,
        image_registry["material_id_depth"],
        pipeline_registry["mat_id_2_depth"],
        global_descset_build_info
    );
    
    material_tile_pipeline = Irori::VirtualGeometry::UniqueMaterialTilePipeline(
        &vulkan_context,
        image_registry["material_id_depth"],
        pipeline_registry["material_tile"],
        global_descset_build_info
    );
}


void UniqueEngine::cpu_frame_render_data_to_staging_buffers(){
    
    // copy instance table to staging buffer
    buffer_registry["instance_table_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        std::memcpy(
            mapped_addr, 
            cpu_frame_render_data.render_instance_table.data(), 
            cpu_frame_render_data.render_instance_table.size()*sizeof(Render::Instance)
        );
        return 0;
    });

    // copy instance reference table to staging buffer
    buffer_registry["instance_reference_table_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* addr = (uint8_t*)mapped_addr;
        
        // common opaque
        size_t copy_size = cpu_frame_render_data.common_opaque_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.common_opaque_instance_refs.data(),  copy_size);
        addr += copy_size;

        // common mask
        copy_size = cpu_frame_render_data.common_mask_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.common_mask_instance_refs.data(),  copy_size);
        addr += copy_size;

        // common blend
        copy_size = cpu_frame_render_data.common_blend_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.common_blend_instance_refs.data(),  copy_size);
        addr += copy_size;

        // vg opaque
        copy_size = cpu_frame_render_data.virtual_geometry_opaque_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.virtual_geometry_opaque_instance_refs.data(),  copy_size);
        addr += copy_size;

        // vg mask
        copy_size = cpu_frame_render_data.virtual_geometry_mask_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.virtual_geometry_mask_instance_refs.data(),  copy_size);
        addr += copy_size;

        // vg blend
        copy_size = cpu_frame_render_data.virtual_geometry_blend_instance_refs.size()*sizeof(Render::InstanceReference);
        std::memcpy(addr, cpu_frame_render_data.virtual_geometry_blend_instance_refs.data(),  copy_size);
        addr += copy_size;

        return 0;
    });

    // copy material data to staging buffer
    buffer_registry["material_table_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* addr = (uint8_t*)mapped_addr;
        
        size_t copy_size = cpu_frame_render_data.material_table.size()*sizeof(Render::Material);
        std::memcpy(addr, cpu_frame_render_data.material_table.data(),  copy_size);

        return 0;
    });

    // copy texture view data to staging buffer
    buffer_registry["texture_view_table_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* addr = (uint8_t*)mapped_addr;
        
        size_t copy_size = cpu_frame_render_data.texture_view_table.size()*sizeof(Render::TextureView);
        std::memcpy(addr, cpu_frame_render_data.texture_view_table.data(),  copy_size);
        
        return 0;
    });


    /////////////////////////////
    // make gpu frame render data   
    Render::GPU_FrameRenderData gpu_frame_render_data;

    gpu_frame_render_data.instance_table = buffer_registry["instance_table"].get_device_address();
    gpu_frame_render_data.instance_reference_table = buffer_registry["instance_reference_table"].get_device_address();
    gpu_frame_render_data.material_table = buffer_registry["material_table"].get_device_address();
    gpu_frame_render_data.texture_view_table = buffer_registry["texture_view_table"].get_device_address();
    
    //////////////////////
    // common opaque range
    Render::InstanceRefRange common_opaque_ref_range;
    common_opaque_ref_range.instance_ref_base = 0;
    common_opaque_ref_range.instance_ref_count = cpu_frame_render_data.common_opaque_instance_refs.size();
    gpu_frame_render_data.common_opaque_instance_ref_range = common_opaque_ref_range;

    // common mask range
    Render::InstanceRefRange common_mask_ref_range;
    common_mask_ref_range.instance_ref_base = common_opaque_ref_range.instance_ref_base + common_opaque_ref_range.instance_ref_count;
    common_mask_ref_range.instance_ref_count = cpu_frame_render_data.common_mask_instance_refs.size();
    gpu_frame_render_data.common_mask_instance_ref_range = common_mask_ref_range;

    // common blend range
    Render::InstanceRefRange common_blend_ref_range;
    common_blend_ref_range.instance_ref_base = common_mask_ref_range.instance_ref_base + common_mask_ref_range.instance_ref_count;
    common_blend_ref_range.instance_ref_count = cpu_frame_render_data.common_blend_instance_refs.size();
    gpu_frame_render_data.common_blend_instance_ref_range = common_blend_ref_range;

    // vg opaque range
    Render::InstanceRefRange vg_opaque_ref_range;
    vg_opaque_ref_range.instance_ref_base = common_blend_ref_range.instance_ref_base + common_blend_ref_range.instance_ref_count;
    vg_opaque_ref_range.instance_ref_count = cpu_frame_render_data.virtual_geometry_opaque_instance_refs.size();
    gpu_frame_render_data.virtual_geometry_opaque_instance_ref_range = vg_opaque_ref_range;

    // vg mask range
    Render::InstanceRefRange vg_mask_ref_range;
    vg_mask_ref_range.instance_ref_base = vg_opaque_ref_range.instance_ref_base + vg_opaque_ref_range.instance_ref_count;
    vg_mask_ref_range.instance_ref_count = cpu_frame_render_data.virtual_geometry_mask_instance_refs.size();
    gpu_frame_render_data.virtual_geometry_mask_instance_ref_range = vg_mask_ref_range;

    // vg blend range
    Render::InstanceRefRange vg_blend_ref_range;
    vg_blend_ref_range.instance_ref_base = vg_mask_ref_range.instance_ref_base + vg_mask_ref_range.instance_ref_count;
    vg_blend_ref_range.instance_ref_count = cpu_frame_render_data.virtual_geometry_blend_instance_refs.size();
    gpu_frame_render_data.virtual_geometry_blend_instance_ref_range = vg_blend_ref_range;
    current_frame_render_instance_reference_count = vg_blend_ref_range.instance_ref_base + vg_blend_ref_range.instance_ref_count;

    /////////
    // camera
    gpu_frame_render_data.camera_position           = cpu_frame_render_data.camera_position;
    gpu_frame_render_data.camera_lookat_d           = cpu_frame_render_data.camera_lookat_d;
    gpu_frame_render_data.camera_vup_vfov           = cpu_frame_render_data.camera_vup_vfov;
    gpu_frame_render_data.view_matrix               = cpu_frame_render_data.view_matrix;
    gpu_frame_render_data.projection_matrix         = cpu_frame_render_data.projection_matrix;
    gpu_frame_render_data.projection_view_matrix    = cpu_frame_render_data.projection_view_matrix;

    /////////
    // light
    gpu_frame_render_data.directional_light_rgba    = cpu_frame_render_data.directional_light_rgba;
    gpu_frame_render_data.directional_light_dir     = cpu_frame_render_data.directional_light_dir;
    gpu_frame_render_data.sky_light_rgba            = cpu_frame_render_data.sky_light_rgba;
    
    ////////////////////
    // render target info
    gpu_frame_render_data.render_target_width   = cpu_frame_render_data.render_target_width;
    gpu_frame_render_data.render_target_height  = cpu_frame_render_data.render_target_height;
    
    ////////////////
    // editor data
    for(int i=0; i<16; i++){
        gpu_frame_render_data.editor_data[i] = cpu_frame_render_data.editor_data[i];
    }

    //////////////////
    // write to staging
    buffer_registry["gpu_frame_render_data_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* addr = (uint8_t*)mapped_addr;
        
        std::memcpy(addr, &gpu_frame_render_data, sizeof(Render::GPU_FrameRenderData));
        
        return 0;
    });
    

}



}