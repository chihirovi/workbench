#include "Engine_DL.hpp"
#include "../Include/ExportImage.hpp"

#define VK_CMD_LABEL_BLOCK(cmd, name, code) \
    do { \
        vk::DebugUtilsLabelEXT label{}; \
        label.setPLabelName(name); \
        (cmd).beginDebugUtilsLabelEXT(label); \
        code; \
        (cmd).endDebugUtilsLabelEXT(); \
    } while(0)

namespace Irori::Engine::DDGI_LOD{


UniqueEngine::UniqueEngine(std::string config_file){
    init_base(config_file);
    printf("hello0\n");
    init_buffers();
    printf("hello1\n");
    init_images();
    printf("hello2\n");
    init_pipelines();
    printf("hello3\n");
}

void UniqueEngine::create_scene_0(){

    float d_light_power = 100.0;
    scene_manager
        .set_directional_light({1.0, 1.0, 1.0, d_light_power}, {0.4, 1.1, 0.4}) //垂直放射照度
        .set_sky_light({1.0, 1.0, 1.0, d_light_power/(2.0*3.1415)});

    scene_manager
        .register_model("red_bunny", "./Assets/RedBunny/bunny.gltf", std::nullopt)
        .register_model("green_bunny", "./Assets/GreenBunny/bunny.gltf", std::nullopt)
        .register_model("blue_bunny", "./Assets/BlueBunny/bunny.gltf", std::nullopt)
        .register_model("sponza", "./Assets/Sponza/Sponza.gltf", std::nullopt)
        ;

    // lod file
    as_lod_files.model_name_2_lod_files["sponza"] = {
        "Assets/SponzaLod1/SponzaLod1.gltf", //lod1
        "Assets/SponzaLod2/SponzaLod2.gltf", //lod2
        "Assets/SponzaLod3/SponzaLod3.gltf", //lod3
        "Assets/SponzaLod4/SponzaLod4.gltf", //lod4
    };
    as_lod_files.model_name_2_lod_files["red_bunny"] = {
        "./Assets/RedBunnyLod1/bunnylod1.gltf", std::nullopt, std::nullopt, std::nullopt    
    };

    as_lod_files.model_name_2_lod_files["green_bunny"] = {
        "./Assets/GreenBunnyLod1/bunnylod1.gltf", std::nullopt, std::nullopt, std::nullopt    
    };

    as_lod_files.model_name_2_lod_files["blue_bunny"] = {
        "./Assets/BlueBunnyLod1/bunnylod1.gltf", std::nullopt, std::nullopt, std::nullopt    
    };


    scene_manager
        .instantiate("red_bunny0", "red_bunny")
        .apply_transform("red_bunny0", glm::translate(glm::mat4(1.0), {1.5, 0.3, 0.0})*glm::scale(glm::mat4(1.0),{5.0, 5.0, 5.0}))
        .instantiate("green_bunny0", "green_bunny")
        .apply_transform("green_bunny0", glm::translate(glm::mat4(1.0), {0.0, 0.3, 0.0})*glm::scale(glm::mat4(1.0),{5.0, 5.0, 5.0}))
        .instantiate("blue_bunny0", "blue_bunny")
        .apply_transform("blue_bunny0", glm::translate(glm::mat4(1.0), {-1.5, 0.3, 0.0})*glm::scale(glm::mat4(1.0),{5.0, 5.0, 5.0}))
        .instantiate("sponza0", "sponza")
        ;

    glm::vec3 lookfrom = {-10.0, 1.23, -0.27};
    glm::vec3 lookdir = {1.072, 0.07, -0.09};

    scene_manager
        .init_camera(render_target_width, render_target_height, lookfrom, lookfrom+lookdir, 45, {0,1,0});
    
        
    ddgi_desc = DDGI::DdgiDesc({0.1, 6.2, 0.1}, {28.0, 14.0, 20.0}, {30, 14, 20}, probe_num_ray);
}


void UniqueEngine::create_scene_1(){

    float d_light_power = 0.3;
    scene_manager
        .set_directional_light({1.0, 1.0, 1.0, d_light_power}, {0.4, 1.1, 0.4}) //垂直放射照度
        .set_sky_light({1.0, 1.0, 1.0, d_light_power/(2.0*3.1415)});

    scene_manager
        .register_model("floor", "./Assets/Plane/plane.gltf", std::nullopt)
        .register_model("tree", "./Assets/Tree0/tree.gltf", std::nullopt)
        .register_model("light_bunny", "./Assets/WhiteBunny/bunny.gltf", std::nullopt)
        ;

    // lod file
    as_lod_files.model_name_2_lod_files["floor"] = {
        "./Assets/Plane/plane.gltf", //lod1
        std::nullopt,
        std::nullopt,
        std::nullopt,
    };
    
    as_lod_files.model_name_2_lod_files["tree"] = {
        "./Assets/Tree0Lod1/tree.gltf", //lod1
        "./Assets/Tree0Lod2/tree.gltf", //lod2
        "./Assets/Tree0Lod3/tree.gltf", //lod3
        std::nullopt,
    };

    as_lod_files.model_name_2_lod_files["light_bunny"] = {
        "./Assets/WhiteBunnyLod1/bunnylod1.gltf", //lod1
        std::nullopt,
        std::nullopt,
        std::nullopt,
    };

    scene_manager
        .instantiate("floor0", "floor")
        .apply_transform("floor0", glm::scale(glm::mat4(1.0), {50.0, 50.0, 50.0})) 
        .instantiate("light_bunny0", "light_bunny")
        .apply_transform("light_bunny0", glm::translate(glm::mat4(1.0), {0.0, 4.0, 7.0}) * glm::scale(glm::mat4(1.0), {20.0, 20.0, 20.0})); 
        ;
    
    float origin_x = 2.0, origin_z = 2.0;
    for(int x=-4; x<4; x++){
        for(int z=-4; z<4; z++){
            float tx = origin_x + x*4.0, tz = origin_z - z*4.0;
            std::string name = "tree" + std::to_string(x) + std::to_string(z);
            scene_manager
                .instantiate(name, "tree")
                .apply_transform(name, glm::translate(glm::mat4(1.0), {tx, 0.0, tz})*glm::scale(glm::mat4(1.0), {0.003, 0.003, 0.003}));
        }
    }
    
    glm::vec3 lookfrom = {0.0, 0.65, 0.0};
    glm::vec3 lookdir = {0.70, -0.3, 1.0};
    scene_manager
        .init_camera(render_target_width, render_target_height, lookfrom, lookfrom+lookdir, 45, {0,1,0});

    ddgi_desc = DDGI::DdgiDesc({2.0, 3.0, 4.0}, {35.0, 7.0, 25.0}, {40, 10, 30}, probe_num_ray);
}

void UniqueEngine::create_scene_2(){

    float d_light_power = 100.0;
    scene_manager
        .set_directional_light({1.0, 1.0, 1.0, d_light_power}, {0.4, 1.1, 0.4}) //垂直放射照度
        .set_sky_light({1.0, 1.0, 1.0, d_light_power/(2.0*3.1415)});

    scene_manager
        .register_model("dh", "./Assets/DamagedHelmet/DamagedHelmet.gltf", std::nullopt)
        ;

    // lod file
    as_lod_files.model_name_2_lod_files["dh"] = {
        "./Assets/DamagedHelmet/DamagedHelmet.gltf", std::nullopt, std::nullopt, std::nullopt    
    };

    scene_manager
        .instantiate("dh0", "dh")
        ;

    scene_manager
        .init_camera(render_target_width, render_target_height, {0,0,0}, {0,0,-1}, 45, {0,1,0});

    ddgi_desc = DDGI::DdgiDesc({0.1, 6.2, 0.1}, {28.0, 14.0, 20.0}, {30, 14, 20}, probe_num_ray);
}

void UniqueEngine::create_scene_3(){

    float d_light_power = 100.0;
    scene_manager
        .set_directional_light({1.0, 1.0, 1.0, d_light_power}, {0.4, 1.1, 0.4}) //垂直放射照度
        .set_sky_light({1.0, 1.0, 1.0, d_light_power/(2.0*3.1415)});

    scene_manager
        .register_model("floor", "./Assets/Plane/plane.gltf", std::nullopt)
        .register_model("sphere", "./Assets/Sphere/scene.gltf", std::nullopt)
        ;

    // lod file
    as_lod_files.model_name_2_lod_files["sphere"] = {
        "./Assets/Sphere/scene.gltf", std::nullopt, std::nullopt, std::nullopt    
    };
    as_lod_files.model_name_2_lod_files["floor"] = {
        "./Assets/Plane/plane.gltf", //lod1
        std::nullopt,
        std::nullopt,
        std::nullopt,
    };

    scene_manager
        .instantiate("sphere0", "sphere")
        .apply_transform("sphere0", glm::scale(glm::mat4(1.0), {0.2, 0.2, 0.2})) 
        //.instantiate("floor0", "floor")
        //.apply_transform("floor0", glm::scale(glm::mat4(1.0), {50.0, 50.0, 50.0})) 
        ;

    scene_manager
        .init_camera(render_target_width, render_target_height, {0,0,0}, {0,0,-1}, 45, {0,1,0});

    ddgi_desc = DDGI::DdgiDesc({0.1, 6.2, 0.1}, {28.0, 14.0, 20.0}, {30, 14, 20}, probe_num_ray);
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
    
    // scene as manager
    //scene_as_manager = Scene::UniqueSceneAsManager(&vulkan_context);
    scene_as_lod_manager = Scene::UniqueSceneAsLodManager(&vulkan_context);
    
    // DDGI
    //ddgi_desc = DDGI::DdgiDesc({0.1, 6.2, 0.1}, {28.0, 14.0, 20.0}, {30, 14, 20});
    
    // editor
    editor = UniqueEditor(&vulkan_context, swapchain);
    
    ////////////////////////////////
    ////////////////////////////////
    ////////////////////////////////
    // test scene //あとで何とかする
    create_scene_0();
    //create_scene_1();
    //create_scene_2();
    //create_scene_3();
}


void UniqueEngine::run(){
    
    uint32_t frame_counter = 0;
    
    // 1) タイムスタンプサポート＆ timestampPeriod 取得
    vk::PhysicalDeviceProperties props = vulkan_context.physical_device.getProperties();
    if (!(props.limits.timestampComputeAndGraphics)) {
        std::printf("This device does not support graphics/compute timestamps.\n");
        exit(-1);
    }
    double timestampPeriod = props.limits.timestampPeriod; // [ns] per tick

    // 2) QueryPool 作成 (2タイムスタンプ用)
    vk::QueryPoolCreateInfo queryPoolInfo{};
    queryPoolInfo.queryType  = vk::QueryType::eTimestamp;
    queryPoolInfo.queryCount = 2;

    vk::UniqueQueryPool queryPool = vulkan_context.device->createQueryPoolUnique(queryPoolInfo);

    // frame rendrer
    auto frame_renderer = [&](vk::CommandBuffer& command_buffer, Core::UniqueImage& final_dst_image){
        if(frame_counter == 0) return;
        //////////////////////////////
        // update AS
        //scene_as_manager.cmd_update_tlas_with_barrier(command_buffer);
        scene_as_lod_manager.cmd_update_tlas_with_barrier(command_buffer);
        
        //////////////////////////////
        // update global descriptor sets
        global_descriptor_sets
            .update("textures", cpu_frame_render_data.texture_table, vk::ImageLayout::eShaderReadOnlyOptimal)
            //.update("accelaration_structure", scene_as_manager.tlas.accel_struct);
            .update("accelaration_structures", scene_as_lod_manager.lod_tlas, 5);
        
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
        
        // rt lod vertex index addr
        // lod 1234
        buffer_registry["rt_lod1234_vert_idx_addr_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["rt_lod1234_vert_idx_addr"], 
            cpu_frame_render_data.rt_lod1234_vert_idx_addr.size()*sizeof(uint64_t)
        );       

        // ddgi desc
        buffer_registry["ddgi_desc_staging"].cmd_copy_to_buffer(
            command_buffer, buffer_registry["ddgi_desc"], 
            sizeof(DDGI::GPU_DDGI_DESC)
        );
        

        //////////////////////////////
        // buffer barrier
        Core::CmdBufferBarrier::transfer_dst_2_task_read(command_buffer, buffer_registry["gpu_frame_render_data"]);
        Core::CmdBufferBarrier::transfer_dst_2_task_read(command_buffer, buffer_registry["instance_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_task_read(command_buffer, buffer_registry["instance_reference_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_task_read(command_buffer, buffer_registry["material_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_task_read(command_buffer, buffer_registry["texture_view_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["rt_lod1234_vert_idx_addr"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["ddgi_desc"]);
        
        //////////////////////////////
        // render
        // common opaque
        bool use_mesh_shader = editor.use_mesh;
        if(use_mesh_shader){
            VK_CMD_LABEL_BLOCK(command_buffer, "mesh_pass",{
                deferred_opaque_mesh_pipeline.cmd_dispatch(
                    command_buffer,  
                    cpu_frame_render_data.common_opaque_instance_refs.size(),
                    global_descriptor_sets
                );
                deferred_mask_mesh_pipeline.cmd_dispatch(
                    command_buffer,  
                    cpu_frame_render_data.common_mask_instance_refs.size(),
                    global_descriptor_sets
                );
            });

        }else{
            VK_CMD_LABEL_BLOCK(command_buffer, "vertex_pass",{
                deferred_opaque_vertex_pipeline.cmd_dispatch(
                    command_buffer,
                    cpu_frame_render_data,
                    global_descriptor_sets
                );
                deferred_mask_vertex_pipeline.cmd_dispatch(
                    command_buffer,
                    cpu_frame_render_data,
                    global_descriptor_sets
                );
            });
        }

        //Query のリセット
        command_buffer.resetQueryPool(*queryPool, 0, 2);

        //タイムスタンプ (開始)
        //TOP_OF_PIPE だとキューに入った順番で "できるだけ早く" のタイミング
        command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, *queryPool, 0);

        // ddgi probe trace
        probe_trace_pipeline.cmd_dispatch(command_buffer, ddgi_desc, global_descriptor_sets);

        command_buffer.writeTimestamp(vk::PipelineStageFlagBits::eAllCommands, *queryPool, 1);
        
        // ddig relocation
        Core::CmdBufferBarrier::rgen_write_2_comp_read(command_buffer, buffer_registry["probe_trace_results"]);
        Core::CmdBufferBarrier::rgen_read_2_comp_write(command_buffer, buffer_registry["probe_relocation_offset"]);
        relocation_pipeline.cmd_dispatch(command_buffer, ddgi_desc, global_descriptor_sets);
        Core::CmdBufferBarrier::comp_write_2_rgen_read(command_buffer, buffer_registry["probe_relocation_offset"]);

        // ddgi irradiance blend
        Core::CmdImageBarrier::rgen_sample_2_comp_read_SRopt2G(command_buffer, image_registry["ddgi_irradiance_texture"]);
        irradiance_blend_pipeline.cmd_dispatch(command_buffer, ddgi_desc, global_descriptor_sets);
        Core::CmdImageBarrier::comp_write_2_rgen_sample_G2SRopt(command_buffer, image_registry["ddgi_irradiance_texture"]);
        
        // ddgi chit t blend
        Core::CmdImageBarrier::rgen_sample_2_comp_read_SRopt2G(command_buffer, image_registry["ddgi_chit_t_texture"]);
        chit_t_blend_pipeline.cmd_dispatch(command_buffer, ddgi_desc, global_descriptor_sets);
        Core::CmdImageBarrier::comp_write_2_rgen_sample_G2SRopt(command_buffer, image_registry["ddgi_chit_t_texture"]);
        
        // irradiance shadow
        Core::CmdImageBarrier::frag_CA_write_2_rgen_read_CAopt2G(command_buffer, image_registry["albedo_gb"]);
        Core::CmdImageBarrier::frag_CA_write_2_rgen_read_CAopt2G(command_buffer, image_registry["world_position_gb"]);
        irradiance_shadow_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        Core::CmdImageBarrier::rgen_write_2_comp_read_G2G(command_buffer, image_registry["diffuse_irradiance_shadow_gb"]);
       
        // pbr lighting
        Core::CmdImageBarrier::frag_CA_write_2_comp_read_CAopt2G(command_buffer, image_registry["normal_occ_gb"]);
        Core::CmdImageBarrier::frag_CA_write_2_comp_read_CAopt2G(command_buffer, image_registry["metal_rough_gb"]);
        Core::CmdImageBarrier::frag_CA_write_2_comp_read_CAopt2G(command_buffer, image_registry["emissive_gb"]);
        pbr_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        
        // debug visualize pass
        bool as_lod_visualize = editor.show_lod;
        bool probe_visualize = editor.show_probe;

        if(as_lod_visualize){ // as lod visualize
            Core::CmdImageBarrier::comp_write_2_rgen_write(command_buffer, image_registry["pbr_output"]);
            as_lod_vis_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
            Core::CmdImageBarrier::rgen_write_2_transfer_G2Tsrc(command_buffer, image_registry["pbr_output"]);

        }else if(probe_visualize){// probe visualize
            Core::CmdImageBarrier::comp_write_2_frag_CA_write_G2CAopt(command_buffer, image_registry["pbr_output"]);
            probe_vis_pipeline.cmd_dispatch(command_buffer, ddgi_desc.get_probe_num(), global_descriptor_sets);
            Core::CmdImageBarrier::frag_CA_write_2_transfer_CAopt2Tsrc(command_buffer, image_registry["pbr_output"]);

        }else{
            Core::CmdImageBarrier::comp_write_2_transfer_G2Tsrc(command_buffer, image_registry["pbr_output"]);
        }
        
        
        // imgui pass
        Core::CmdImageBarrier::top_pipe_2_transfer_CAopt2Tdst(command_buffer, editor.render_image);
        image_registry["pbr_output"].cmd_copy_to_image(command_buffer, editor.render_image);
        Core::CmdImageBarrier::transfer_2_frag_write_CA_Tdst2CAopt(command_buffer, editor.render_image);
        editor.cmd_render(command_buffer, scene_manager, ddgi_desc);

        // copy image pass
        Core::CmdImageBarrier::top_pipe_2_transfer_Psrc2Tdst(command_buffer, final_dst_image);
        Core::CmdImageBarrier::frag_CA_write_2_transfer_CAopt2Tsrc(command_buffer, editor.render_image);
        editor.render_image.cmd_copy_to_image(command_buffer, final_dst_image);
        

        // イメージのレイアウトを元に戻す
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tsrc2G(command_buffer, image_registry["pbr_output"]);
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tdst2Psrc(command_buffer, final_dst_image);
        Core::CmdImageBarrier::transfer_2_bottom_pipe_Tdst2fragCA(command_buffer, editor.render_image);
        Core::CmdImageBarrier::comp_read_2_bottom_pipe_G2CAopt(command_buffer, image_registry["albedo_gb"]);
        Core::CmdImageBarrier::comp_read_2_bottom_pipe_G2CAopt(command_buffer, image_registry["world_position_gb"]);
        Core::CmdImageBarrier::comp_read_2_bottom_pipe_G2CAopt(command_buffer, image_registry["normal_occ_gb"]);
        Core::CmdImageBarrier::comp_read_2_bottom_pipe_G2CAopt(command_buffer, image_registry["metal_rough_gb"]);
        Core::CmdImageBarrier::comp_read_2_bottom_pipe_G2CAopt(command_buffer, image_registry["emissive_gb"]);
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
        if(editor.is_using_guizmo == false && editor.is_using_gui == false){
            scene_manager.update_camera(window.time_delta(), key_response, mouse_offset);
        }
        if(editor.camera_reset) scene_manager.camera.reset();
        
        cpu_frame_render_data.editor_data[0] = editor.clear_probe ? 1 : 0;
        cpu_frame_render_data.editor_data[1] = editor.lod;
        cpu_frame_render_data.editor_data[2] = editor.selected_lock_lod;
        cpu_frame_render_data.editor_data[3] = editor.screen_shot_tonemap_srgb ? 1 : editor.screen_shot_raw_radiance ? 2 : 0;
        cpu_frame_render_data.editor_data[4] = editor.is_transpose ? 1 : 0;
        cpu_frame_render_data.editor_data[5] = 10;
        cpu_frame_render_data.editor_data[6] = 11;
        cpu_frame_render_data.editor_data[7] = 12;

        // build cpu frame render data
        cpu_frame_render_data.reset();

        scene_manager.build_cpu_frame_render_data(&cpu_frame_render_data);

        if(!cpu_frame_render_data.is_need_to_rebuild_AS){
            /*
            scene_as_manager.prepare_tlas_update_data(scene_manager);
            scene_as_manager.build_cpu_frame_render_data(&cpu_frame_render_data);
            */
            scene_as_lod_manager.prepare_tlas_update_data(scene_manager);
            //scene_as_lod_manager.build_cpu_frame_render_data(&cpu_frame_render_data);
        }

        cpu_frame_render_data.render_target_width = render_target_width;
        cpu_frame_render_data.render_target_height = render_target_height;
        
        // transfer data to staging buffers
        scene_as_lod_manager.build_cpu_frame_render_data(&cpu_frame_render_data);
        cpu_frame_render_data_to_staging_buffers();
        
        // ddgi update
        ddgi_desc.update_probe_ray_rotation();
        ddgi_desc.ddgi_desc_to_staging_buffer(buffer_registry["ddgi_desc_staging"]);
    };

    auto cpu_task_after_rendering = [&](float frame_time_ms){
        //editor
        editor.frame_time_ms = frame_time_ms;

        // rebuild as
        bool need_to_rebuild_cpu_frame_render_data = false; 
        
        if(cpu_frame_render_data.is_need_to_rebuild_AS){
            /*
            scene_as_manager.rebuild_tlas_blas(scene_manager, cpu_frame_render_data);
            scene_as_manager.prepare_tlas_update_data(scene_manager);
            scene_as_manager.build_cpu_frame_render_data(&cpu_frame_render_data);
            */
            scene_as_lod_manager.rebuild_tlas_blas(scene_manager, cpu_frame_render_data, as_lod_files);
            scene_as_lod_manager.prepare_tlas_update_data(scene_manager);
            scene_as_lod_manager.build_cpu_frame_render_data(&cpu_frame_render_data);
            need_to_rebuild_cpu_frame_render_data = true;
        }
        
        cpu_frame_render_data_to_staging_buffers();
        
        
        if(frame_counter % 60 == 0){
            if(frame_counter == 60) scene_manager.camera.reset();
            /*
            printf("%d\n", cpu_frame_render_data.render_instance_table.size());
            for(auto [mesh_id, prim_id] : scene_manager.render_inst_table_gltf_mesh_prim_id_map){
                printf("%d %d\n",mesh_id, prim_id);
            }
            printf("--------------------\n\n");
            */
            //printf("blend %d, mask %d, opaque %d\n", cpu_frame_render_data.common_blend_instance_refs.size(), cpu_frame_render_data.common_mask_instance_refs.size(), cpu_frame_render_data.common_opaque_instance_refs.size());
        }

        // probe trace time -> editor
        if(frame_counter > 0){
            uint64_t timestamps[2] = {};
            vulkan_context.device->getQueryPoolResults(
                *queryPool,
                0,
                2,
                sizeof(timestamps), 
                timestamps, 
                sizeof(uint64_t), 
                vk::QueryResultFlagBits::e64|vk::QueryResultFlagBits::eWait
            );
            
            uint64_t deltaTicks = timestamps[1] - timestamps[0];
            double   deltaNs    = static_cast<double>(deltaTicks) * timestampPeriod;
            double   deltaMs    = deltaNs / 1.0e6;
            editor.probe_trace_time = deltaMs;
            //std::cout << "vkCmdTraceRaysKHR GPU time: " << deltaMs << " ms ("<< deltaTicks << " ticks, " << timestampPeriod << " ns/tick)\n";
        }
        
        // export image
        if(editor.export_screen_shot_tonemap_srgb) export_tonemap_srgb("tone_srgb.ppm", (float*)(buffer_registry["screen_shot_buffer"].get_mapped_addr_for_read()), render_target_width, render_target_height);
        if(editor.export_screen_shot_raw_radiance) export_raw_radiance("raw_radiance.log", (float*)(buffer_registry["screen_shot_buffer"].get_mapped_addr_for_read()), render_target_width, render_target_height);
        auto as_size_temp = scene_as_lod_manager.get_all_AS_size(scene_manager);
        editor.lod0_as_size = as_size_temp.first;
        editor.lod1234_as_size = as_size_temp.second;
        frame_counter++;
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
    
    // rt lod1234 vert idx addr 
    buffer_registry.reg_resource("rt_lod1234_vert_idx_addr");
    buffer_registry["rt_lod1234_vert_idx_addr"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(uint64_t)*2*MAX_RENDER_INSTANCE_NUM,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eShaderDeviceAddress
    );
    buffer_registry.reg_resource("rt_lod1234_vert_idx_addr_staging");
    buffer_registry["rt_lod1234_vert_idx_addr_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(uint64_t)*2*MAX_RENDER_INSTANCE_NUM
    );

    // DDGI probe trace result buffer
    buffer_registry.reg_resource("probe_trace_results");
    buffer_registry["probe_trace_results"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(float)*4*ddgi_desc.probe_num_ray*ddgi_desc.get_probe_num(),
        false,
        vk::BufferUsageFlagBits::eStorageBuffer
    );

    // DDGI probe offset (for relocation)
    buffer_registry.reg_resource("probe_relocation_offset");
    buffer_registry["probe_relocation_offset"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(float)*4*ddgi_desc.get_probe_num(),
        false,
        vk::BufferUsageFlagBits::eStorageBuffer
    );
    
    // DDGI ddgi desc
    buffer_registry.reg_resource("ddgi_desc");
    buffer_registry["ddgi_desc"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(DDGI::GPU_DDGI_DESC),
        false,
        vk::BufferUsageFlagBits::eUniformBuffer|vk::BufferUsageFlagBits::eTransferDst
    );
    buffer_registry.reg_resource("ddgi_desc_staging");
    buffer_registry["ddgi_desc_staging"] = Core::UniqueBuffer::upload(
        &vulkan_context,
        sizeof(DDGI::GPU_DDGI_DESC)
    );
    
    // screen shot buffer
    buffer_registry.reg_resource("screen_shot_buffer");
    buffer_registry["screen_shot_buffer"] = Core::UniqueBuffer::download(
        &vulkan_context,
        sizeof(float)*3*render_target_width*render_target_height, //rgb
        vk::BufferUsageFlagBits::eStorageBuffer
    );
}


void UniqueEngine::init_images(){
    // albedo Gbuffer
    image_registry.reg_resource("albedo_gb");
    image_registry["albedo_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();
    
    // world_position Gbuffer
    image_registry.reg_resource("world_position_gb");
    image_registry["world_position_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();
    
    // normal Gbuffer
    image_registry.reg_resource("normal_occ_gb");
    image_registry["normal_occ_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();
    
    // metal rough occ Gbuffer
    image_registry.reg_resource("metal_rough_gb");
    image_registry["metal_rough_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8Unorm,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();

    // emissive Gbuffer
    image_registry.reg_resource("emissive_gb");
    image_registry["emissive_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eColorAttachment,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();

    // depth
    image_registry.reg_resource("depth_st");
    image_registry["depth_st"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eD32Sfloat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            vk::ImageLayout::eDepthStencilAttachmentOptimal
        )
        .build();
    
    // pbr output image
    image_registry.reg_resource("pbr_output");
    image_registry["pbr_output"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eGeneral
        )
        .build();
    
    // DDGI irradiance texture
    auto probe_nums = ddgi_desc.get_probe_nums();
    image_registry.reg_resource("ddgi_irradiance_texture");
    image_registry["ddgi_irradiance_texture"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            probe_nums.x*DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER, probe_nums.y*DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER,
            //vk::Format::eB10G11R11UfloatPack32, //ちゃんとエンコードしないと精度が足りない
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eShaderReadOnlyOptimal
        )
        .set_2d_array_size(probe_nums.z)
        .build();
    
    // DDGI chit_t texture
    image_registry.reg_resource("ddgi_chit_t_texture");
    image_registry["ddgi_chit_t_texture"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            probe_nums.x*DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER, probe_nums.y*DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER,
            vk::Format::eR16G16Sfloat,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eShaderReadOnlyOptimal
        )
        .set_2d_array_size(probe_nums.z)
        .build();
    
    // DDGI irradiance shadow 
    image_registry.reg_resource("diffuse_irradiance_shadow_gb");
    image_registry["diffuse_irradiance_shadow_gb"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR16G16B16A16Sfloat,
            vk::ImageUsageFlagBits::eStorage,
            vk::ImageLayout::eGeneral
        )
        .build();
}


void UniqueEngine::init_pipelines(){
    //create pipeline registry
    Irori::Core::ShaderReflect::PipelineRegistry pipeline_registry;

    // deferred opaque mesh
    pipeline_registry.register_pipeline_name("deferred_opaque_mesh");
    pipeline_registry["deferred_opaque_mesh"]
    .add_shader_info(
        "./Shaders/Spv/DeferredTaskOpaque.spv",
        "main_task"
    ).add_shader_info(
        "./Shaders/Spv/DeferredMesh.spv",
        "main_mesh"
    ).add_shader_info(
        "./Shaders/Spv/DeferredFragOpaque.spv",
        "main_fragment"
    );
    
    // deferred mask mesh
    pipeline_registry.register_pipeline_name("deferred_mask_mesh");
    pipeline_registry["deferred_mask_mesh"]
    .add_shader_info(
        "./Shaders/Spv/DeferredTaskMask.spv",
        "main_task"
    ).add_shader_info(
        "./Shaders/Spv/DeferredMesh.spv",
        "main_mesh"
    ).add_shader_info(
        "./Shaders/Spv/DeferredFragMask.spv",
        "main_fragment"
    );
    
    // deferred opaque vertex
    pipeline_registry.register_pipeline_name("deferred_opaque_vertex");
    pipeline_registry["deferred_opaque_vertex"]
    .add_shader_info(
        "./Shaders/Spv/DeferredVertexOpaque.spv",
        "main_vert"
    ).add_shader_info(
        "./Shaders/Spv/DeferredVertexOpaque.spv",
        "main_fragment"
    );
    
    // deferred mask vertex
    pipeline_registry.register_pipeline_name("deferred_mask_vertex");
    pipeline_registry["deferred_mask_vertex"]
    .add_shader_info(
        "./Shaders/Spv/DeferredVertexMask.spv",
        "main_vert"
    ).add_shader_info(
        "./Shaders/Spv/DeferredVertexMask.spv",
        "main_fragment"
    );
    
    // pbr lighting
    pipeline_registry.register_pipeline_name("pbr_lighting");
    pipeline_registry["pbr_lighting"]
    .add_shader_info(
        "./Shaders/Spv/Pbr.spv",
        "main_comp"
    );
    
    // DDGI Probe Trace
    pipeline_registry.register_pipeline_name("ddgi_probe_trace");
    pipeline_registry["ddgi_probe_trace"]
    .add_shader_info(
        "./Shaders/Spv/ProbeTrace.spv",
        "main_rgen_probe_trace"
    ).add_shader_info(
        "./Shaders/Spv/ProbeTrace.spv",
        "main_miss_probe_trace"
    ).add_shader_info(
        "./Shaders/Spv/ProbeTrace.spv",
        "main_chit_probe_trace"
    ).add_shader_info(
        "./Shaders/Spv/ProbeTrace.spv",
        "main_ahit_probe_trace"
    );  
    
    // DDGI irradiance blend
    pipeline_registry.register_pipeline_name("irradiance_blend");
    pipeline_registry["irradiance_blend"]
    .add_shader_info(
        "./Shaders/Spv/ProbeIrradianceBlend.spv",
        "main_comp"
    );   
    
    // DDGI chit t blend
    pipeline_registry.register_pipeline_name("chit_t_blend");
    pipeline_registry["chit_t_blend"]
    .add_shader_info(
        "./Shaders/Spv/ProbeChitBlend.spv",
        "main_comp"
    );   
    
    // DDGI calc irradiance shadow
    pipeline_registry.register_pipeline_name("irradiance_shadow");
    pipeline_registry["irradiance_shadow"]
    .add_shader_info(
        "./Shaders/Spv/DiffuseIrradianceShadow.spv",
        "main_rgen"
    ).add_shader_info(
        "./Shaders/Spv/DiffuseIrradianceShadow.spv",
        "main_miss"
    ).add_shader_info(
        "./Shaders/Spv/DIffuseIrradianceShadow.spv",
        "main_chit"
    ).add_shader_info(
        "./Shaders/Spv/DIffuseIrradianceShadow.spv",
        "main_ahit"
    );  
    
    // probe visualize
    pipeline_registry.register_pipeline_name("probe_visualize");
    pipeline_registry["probe_visualize"]
    .add_shader_info(
        "./Shaders/Spv/ProbeVisualize.spv",
        "main_vert"
    ).add_shader_info(
        "./Shaders/Spv/ProbeVisualize.spv",
        "main_frag"
    );  
    
    // ddgi relocation
    pipeline_registry.register_pipeline_name("probe_relocation");
    pipeline_registry["probe_relocation"]
    .add_shader_info(
        "./Shaders/Spv/ProbeRelocation.spv",
        "main_comp"
    );

    // lod as visualize
    pipeline_registry.register_pipeline_name("as_lod_vis");
    pipeline_registry["as_lod_vis"]
    .add_shader_info(
        "./Shaders/Spv/AsLodVisualize.spv",
        "main_rgen"
    ).add_shader_info(
        "./Shaders/Spv/AsLodVisualize.spv",
        "main_miss"
    ).add_shader_info(
        "./Shaders/Spv/AsLodVisualize.spv",
        "main_chit"
    ).add_shader_info(
        "./Shaders/Spv/AsLodVisualize.spv",
        "main_ahit"
    );

    // build global descriptor set
    auto global_descset_build_info = Irori::Core::ShaderReflect::extract_global_and_build_local_descsets_info(pipeline_registry);

    global_descriptor_sets = Irori::Core::UniqueDescriptorSets::Global(
        &vulkan_context,
        global_descset_build_info
    );
    
    //////////////////////
    // bind resource to global descriptor set
    auto samplers = sampler_pool.get_samplers();
    global_descriptor_sets
        .update("frame_render_data", buffer_registry["gpu_frame_render_data"])
        .update("samplers", samplers)
        .update("albedo_gb", image_registry["albedo_gb"], vk::ImageLayout::eGeneral)
        .update("world_position_gb", image_registry["world_position_gb"], vk::ImageLayout::eGeneral)
        .update("normal_occ_gb", image_registry["normal_occ_gb"], vk::ImageLayout::eGeneral)
        .update("metal_rough_gb", image_registry["metal_rough_gb"], vk::ImageLayout::eGeneral)
        .update("emissive_gb", image_registry["emissive_gb"], vk::ImageLayout::eGeneral)
        .update("ddgi_desc", buffer_registry["ddgi_desc"])
        .update("probe_irradiance_texture", image_registry["ddgi_irradiance_texture"], vk::ImageLayout::eShaderReadOnlyOptimal)
        .update("probe_irradiance_texture_general", image_registry["ddgi_irradiance_texture"], vk::ImageLayout::eGeneral)
        .update("probe_chit_t_texture", image_registry["ddgi_chit_t_texture"], vk::ImageLayout::eShaderReadOnlyOptimal)
        .update("probe_chit_t_texture_general", image_registry["ddgi_chit_t_texture"], vk::ImageLayout::eGeneral)
        .update("probe_trace_results", buffer_registry["probe_trace_results"])
        .update("diffuse_irradiance_shadow_gb", image_registry["diffuse_irradiance_shadow_gb"], vk::ImageLayout::eGeneral)
        .update("probe_relocation_offset", buffer_registry["probe_relocation_offset"])
        .update("screen_shot_buffer", buffer_registry["screen_shot_buffer"]);

    ////////////////////
    // deffered opaque mesh pipeline
    deferred_opaque_mesh_pipeline = Deferred::UniqueOpaqueMeshPipeline(
        &vulkan_context,
        image_registry["albedo_gb"],
        image_registry["world_position_gb"],
        image_registry["normal_occ_gb"],
        image_registry["metal_rough_gb"],
        image_registry["emissive_gb"],
        image_registry["depth_st"],
        pipeline_registry["deferred_opaque_mesh"],
        global_descset_build_info
    );

    // deffered opaque mesh pipeline
    deferred_mask_mesh_pipeline = Deferred::UniqueMaskMeshPipeline(
        &vulkan_context,
        image_registry["albedo_gb"],
        image_registry["world_position_gb"],
        image_registry["normal_occ_gb"],
        image_registry["metal_rough_gb"],
        image_registry["emissive_gb"],
        image_registry["depth_st"],
        pipeline_registry["deferred_mask_mesh"],
        global_descset_build_info
    );
 
    ////////////////////
    // deffered opaque vertex pipeline
    deferred_opaque_vertex_pipeline = Deferred::UniqueOpaqueVertexPipeline(
        &vulkan_context,
        image_registry["albedo_gb"],
        image_registry["world_position_gb"],
        image_registry["normal_occ_gb"],
        image_registry["metal_rough_gb"],
        image_registry["emissive_gb"],
        image_registry["depth_st"],
        pipeline_registry["deferred_opaque_vertex"],
        global_descset_build_info
    );   

    // deffered opaque vertex pipeline
    deferred_mask_vertex_pipeline = Deferred::UniqueMaskVertexPipeline(
        &vulkan_context,
        image_registry["albedo_gb"],
        image_registry["world_position_gb"],
        image_registry["normal_occ_gb"],
        image_registry["metal_rough_gb"],
        image_registry["emissive_gb"],
        image_registry["depth_st"],
        pipeline_registry["deferred_mask_vertex"],
        global_descset_build_info
    );   

    ////////////////////////
    // pbr pipeline
    pbr_pipeline = Pbr::UniquePbrPipeline(
        &vulkan_context,
        image_registry["pbr_output"],
        pipeline_registry["pbr_lighting"],
        global_descset_build_info
    );
    
    ///////////////////////
    // ddgi probe trace
    probe_trace_pipeline = DDGI::UniqueProbeTracePipeline(
        &vulkan_context,
        pipeline_registry["ddgi_probe_trace"],
        global_descset_build_info
    );

    // ddgi irradiance blend
    irradiance_blend_pipeline = DDGI::UniqueBlendPipeline(
        &vulkan_context,
        pipeline_registry["irradiance_blend"],
        global_descset_build_info
    );
    
    // ddgi chit t blend
    chit_t_blend_pipeline = DDGI::UniqueBlendPipeline(
        &vulkan_context,
        pipeline_registry["chit_t_blend"],
        global_descset_build_info
    );
    
    // ddge relocation 
    relocation_pipeline = DDGI::UniqueRelocationPipeline(
        &vulkan_context,
        pipeline_registry["probe_relocation"],
        global_descset_build_info
    );
    
    // irradiance shadow
    irradiance_shadow_pipeline = DDGI::UniqueIrradianceShadowPipeline(
        &vulkan_context,
        pipeline_registry["irradiance_shadow"],
        global_descset_build_info
    );
    
    // probe visualize
    probe_vis_pipeline = DDGI::UniqueProbeVisPipeline(
        &vulkan_context,
        image_registry["pbr_output"],
        image_registry["depth_st"],
        pipeline_registry["probe_visualize"],
        global_descset_build_info
    );
    
    as_lod_vis_pipeline = DDGI::UniqueAsLodVisualizePipeline(
        &vulkan_context,
        image_registry["pbr_output"],
        pipeline_registry["as_lod_vis"],
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
    
    // lod1234 vertex index addr
    buffer_registry["rt_lod1234_vert_idx_addr_staging"].write_data_to_host_visible_mem([&](void* mapped_addr){
        uint8_t* addr = (uint8_t*)mapped_addr;
        
        size_t copy_size = cpu_frame_render_data.rt_lod1234_vert_idx_addr.size()*sizeof(uint64_t);
        std::memcpy(addr, cpu_frame_render_data.rt_lod1234_vert_idx_addr.data(),  copy_size);
        
        return 0;
    });

    /////////////////////////////
    // make gpu frame render data   
    Render::GPU_FrameRenderData gpu_frame_render_data;

    gpu_frame_render_data.instance_table = buffer_registry["instance_table"].get_device_address();
    gpu_frame_render_data.instance_reference_table = buffer_registry["instance_reference_table"].get_device_address();
    gpu_frame_render_data.material_table = buffer_registry["material_table"].get_device_address();
    gpu_frame_render_data.texture_view_table = buffer_registry["texture_view_table"].get_device_address();
    gpu_frame_render_data.rt_lod1234_vert_idx_addr = buffer_registry["rt_lod1234_vert_idx_addr"].get_device_address();
    
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