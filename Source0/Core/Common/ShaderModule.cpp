#include "ShaderModule.hpp"

namespace Irori::Core{
    
UniqueShaderModules::UniqueShaderModules(
    VulkanContext* vc, 
    std::vector<ShaderReflect::ShaderInfo>& shader_infos){
    
    vulkan_context = vc;
    
    create_shader_modules(shader_infos);
}

void UniqueShaderModules::create_shader_modules(std::vector<ShaderReflect::ShaderInfo>& shader_infos){

    for(auto& shader_info : shader_infos){

        shader_modules.push_back(create_shader_module(shader_info.spv_path));

        pipeline_shader_create_infos.push_back(
            vk::PipelineShaderStageCreateInfo()
                .setStage(shader_info.stage_flag)
                .setModule(*shader_modules.back())
                .setPName(shader_info.entry_name.c_str())
        );
    }    
}

vk::UniqueShaderModule UniqueShaderModules::create_shader_module(std::string file_path){
    /*check file existing*/
    ASSERT_WITH_MSG(std::filesystem::exists(file_path), std::format("file doesn't exist [{}]", file_path));

    /*read file to u8 buffer*/
    size_t spvfile_size = std::filesystem::file_size(file_path);
    std::ifstream spv_file(file_path, std::ios_base::binary);
    std::vector<char> spvfile_data(spvfile_size);
    spv_file.read(spvfile_data.data(), spvfile_size);
    
    /*create shader module*/
    return vulkan_context->device->createShaderModuleUnique(
        vk::ShaderModuleCreateInfo()
            .setCodeSize(spvfile_size) 
            .setPCode((const uint32_t*)spvfile_data.data())
    );
}

}