#pragma once
#include "VulkanContext.hpp"
#include "ShaderReflect.hpp"
#include <filesystem>
#include <fstream>

namespace Irori::Core{

class UniqueShaderModules{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueShaderModules);

public:
    VulkanContext* vulkan_context = nullptr;
    std::vector<vk::UniqueShaderModule> shader_modules;
    std::vector<vk::PipelineShaderStageCreateInfo> pipeline_shader_create_infos;

public:
    UniqueShaderModules(){;}
    UniqueShaderModules(VulkanContext* vc, std::vector<ShaderReflect::ShaderInfo>& shader_infos);
    vk::UniqueShaderModule create_shader_module(std::string file_path);
    void create_shader_modules(std::vector<ShaderReflect::ShaderInfo>& shader_infos);
    UniqueShaderModules(BOOST_RV_REF(UniqueShaderModules) rhs) = default;               //move constractor
    UniqueShaderModules& operator=(BOOST_RV_REF(UniqueShaderModules) rhs) = default;    //move assignment
};
    
}