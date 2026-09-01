#include "Engine_PT.hpp"


namespace Irori::Engine::PathTrace{

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
    
    // scene as manager
    scene_as_manager = Scene::UniqueSceneAsManager(&vulkan_context);
    
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

    scene_manager
        /*
        .register_model("room", "./Assets/cornell_box/scene_room_only.gltf", std::nullopt)
        .register_model("room_test", "./Assets/cornell_box/scene.gltf", std::nullopt)
        .register_model("light", "./Assets/cornell_box/light_plane.gltf", std::nullopt)
        .register_model("sphere", "./Assets/Sphere/scene.gltf", std::nullopt)
        .register_model("sphere_type1", "./Assets/SphereType1/scene.gltf", std::nullopt) //metal rough -> 0.0, 0.0
        .register_model("sphere_type2", "./Assets/SphereType2/scene.gltf", std::nullopt) //metal rough -> 1.0, 0.5
        .register_model("sphere_type3", "./Assets/SphereType3/scene.gltf", std::nullopt) //metal rough -> 1.0, 0.0
        */
        .register_model("bunny", "./Assets/Bunny/bunny.gltf", std::nullopt)
        .register_model("light", "./Assets/cornell_box/light_plane.gltf", std::nullopt)
        //.register_model("sponza", "./Assets/Sponza/Sponza.gltf", std::nullopt)
        ;
    
    //scene_manager.model_manager.print_mesh_data("room_test");

    scene_manager
        /*
        .instantiate("room0", "room")
        .instantiate("light0", "light")
        .apply_transform("light0", glm::translate(glm::f32mat4x4(1.0), {0.0f, 1.98f, 0.0f}))
        .instantiate("sphere0", "sphere")
        .apply_transform("sphere0", glm::translate(glm::scale(glm::f32mat4x4(0.9), {0.3, 0.3, 0.3}), {-2.2f, 0.0f, 1.0f}))
        .instantiate("sphere1", "sphere_type1")
        .apply_transform("sphere1", glm::translate(glm::scale(glm::f32mat4x4(0.9), {0.3, 0.3, 0.3}), {0.0f, 0.0f, 1.0f}))
        .instantiate("sphere2", "sphere_type2")
        .apply_transform("sphere2", glm::translate(glm::scale(glm::f32mat4x4(0.9), {0.3, 0.3, 0.3}), {2.2f, 0.0f, 1.0f}))
        .instantiate("sphere3", "sphere_type3")
        .apply_transform("sphere3", glm::translate(glm::scale(glm::f32mat4x4(0.9), {0.3, 0.3, 0.3}), {0.0f, 2.0f, -0.5f}))
        */
        .instantiate("bunny0", "bunny")
        .apply_transform("bunny0", glm::rotate(glm::scale(glm::f32mat4x4(1.0), {4.0f, 4.0f, 4.0f}), 3.14f/2.0f, {1.0f, 0.0f, 0.0f}))
        .instantiate("light0", "light")
        .apply_transform("light0", glm::translate(glm::f32mat4x4(1.0), {0.0f, 1.98f, 0.0f}))
        ;
    

    glm::f32vec3 look_from = {0.0f, 1.0f, 4.0f};
    glm::f32vec3 look_at_d = {0.0f, 0.0f, -1.0f};
    look_at_d = glm::normalize(look_at_d);
    scene_manager
        .init_camera(render_target_width, render_target_height, look_from, look_from + look_at_d, 45.0f, {0,1,0});

    // light tile info
    auto light_triangles_light = CalcLightTileInfo(scene_manager, "light0", "./Assets/cornell_box/light_plane.gltf");
    
    light_tile_info.push_back(float(light_triangles_light.first.size()/3));
    light_tile_info.push_back(light_triangles_light.second[0] * light_triangles_light.second[3]);
    light_tile_info.push_back(light_triangles_light.second[1] * light_triangles_light.second[3]);
    light_tile_info.push_back(light_triangles_light.second[2] * light_triangles_light.second[3]);
    for(auto& triangle : light_triangles_light.first){
        light_tile_info.push_back(triangle.x);
        light_tile_info.push_back(triangle.y);
        light_tile_info.push_back(triangle.z);
    }
    
    printf("%f\n", light_tile_info[0]);
    printf("%f %f %f\n", light_tile_info[1], light_tile_info[2], light_tile_info[3]);
    
    for(int i=0; i<light_tile_info.size()-4; i += 3){
        printf("%f %f %f\n", light_tile_info[4 + i + 0], light_tile_info[4 + i + 1], light_tile_info[4 + i + 2]);
    }
}


void UniqueEngine::run(){
    
    uint32_t frame_counter = 0;

    // 本当はここでこんなことするべきじゃないが
    buffer_registry["light_triangles"].transfer_data_to_device_mem(
        light_tile_info.data(),
        light_tile_info.size()*sizeof(float)
    );

    // frame rendrer
    auto frame_renderer = [&](vk::CommandBuffer& command_buffer, Core::UniqueImage& final_dst_image){

        if(frame_counter == 0) return;


        //////////////////////////////
        // update AS
        scene_as_manager.cmd_update_tlas_with_barrier(command_buffer);
        
        //////////////////////////////
        // update global descriptor sets
        global_descriptor_sets
            .update("textures", cpu_frame_render_data.texture_table, vk::ImageLayout::eShaderReadOnlyOptimal)
            .update("accelaration_structures", scene_as_manager.tlas.accel_struct);
        
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
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["gpu_frame_render_data"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["instance_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["instance_reference_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["material_table"]);
        Core::CmdBufferBarrier::transfer_dst_2_raygen_read(command_buffer, buffer_registry["texture_view_table"]);
        
        //////////////////////////////
        // path trace
        pt_pipeline.cmd_dispatch(command_buffer, render_target_width, render_target_height, global_descriptor_sets);
        
        // image barrier
        Core::CmdImageBarrier::rgen_write_2_transfer_G2Tsrc(command_buffer, image_registry["output"]);
        
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
        
        cpu_frame_render_data.editor_data[0] = editor.reset_pt_acc ? 1 : 0;
        cpu_frame_render_data.editor_data[1] = editor.frame_conut;

        // build cpu frame render data
        cpu_frame_render_data.reset();

        scene_manager.build_cpu_frame_render_data(&cpu_frame_render_data);

        if(!cpu_frame_render_data.is_need_to_rebuild_AS){
            scene_as_manager.prepare_tlas_update_data(scene_manager);
        }

        cpu_frame_render_data.render_target_width = render_target_width;
        cpu_frame_render_data.render_target_height = render_target_height;
        
        // transfer data to staging buffers
        cpu_frame_render_data_to_staging_buffers();
    };

    auto cpu_task_after_rendering = [&](float frame_time_ms){
        //editor
        editor.frame_time_ms = frame_time_ms;

        // rebuild as
        bool need_to_rebuild_cpu_frame_render_data = false; 
        
        if(cpu_frame_render_data.is_need_to_rebuild_AS){
            scene_as_manager.rebuild_tlas_blas(scene_manager, cpu_frame_render_data);
            scene_as_manager.prepare_tlas_update_data(scene_manager);
            need_to_rebuild_cpu_frame_render_data = true;
        }
        
        if(need_to_rebuild_cpu_frame_render_data){
            cpu_frame_render_data_to_staging_buffers();
        }
        
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
    
    // light tile buffer
    buffer_registry.reg_resource("light_triangles");
    buffer_registry["light_triangles"] = Core::UniqueBuffer::common(
        &vulkan_context,
        sizeof(float)*light_tile_info.size(),
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eTransferDst
    );   
}


void UniqueEngine::init_images(){
    // output image
    image_registry.reg_resource("acc_image");
    image_registry["acc_image"] = Core::UniqueImage::Builder2D(&vulkan_context)
        .set_size_format_usage_layout(
            render_target_width, render_target_height,
            vk::Format::eR32G32B32A32Sfloat,
            vk::ImageUsageFlagBits::eStorage|vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eSampled,
            vk::ImageLayout::eGeneral
        )
        .build();

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
}


void UniqueEngine::init_pipelines(){
    //create pipeline registry
    Irori::Core::ShaderReflect::PipelineRegistry pipeline_registry;
    
    // DDGI Probe Trace
    pipeline_registry.register_pipeline_name("pt");
    pipeline_registry["pt"]
    .add_shader_info(
        "./Shaders/Spv/pt.spv",
        "main_rgen"
    ).add_shader_info(
        "./Shaders/Spv/pt.spv",
        "main_miss"
    ).add_shader_info(
        "./Shaders/Spv/pt.spv",
        "main_chit"
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
        .update("samplers", samplers);
    
    ///////////////////////
    // ddgi probe trace
    pt_pipeline = Irori::PathTrace::UniquePtPipeline(
        &vulkan_context,
        pipeline_registry["pt"],
        global_descset_build_info,
        image_registry["acc_image"],
        image_registry["output"],
        buffer_registry["light_triangles"]
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