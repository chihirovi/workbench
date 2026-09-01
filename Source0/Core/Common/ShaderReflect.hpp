#pragma once
#include "VulkanContext.hpp"
#include <spirv_cross/spirv_glsl.hpp>
#include <string>
#include <unordered_map>

#define IRORI_BINDLESS_DESC_MAX_SIZE 1024

namespace Irori::Core::ShaderReflect{

//------------------
//struct : descriptor binding info
//------------------
struct DescriptorBindingInfo{
    std::string name;
    vk::DescriptorType type;
    uint32_t   descriptor_count;
    vk::ShaderStageFlags stage_frags;
};

//------------------
//struct : Descsets Info
//------------------
struct DescsetsInfo{
public:
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, DescriptorBindingInfo>> core;

public:
    std::unordered_map<uint32_t, DescriptorBindingInfo>& operator[](uint32_t set_num);
    bool is_empty_set(uint32_t set);
    bool is_empty_set_bind(uint32_t set, uint32_t bind);
    void print();
};

//------------------
//struct : shader info
//------------------
//一つのshader fileに対応する
struct ShaderInfo{
    std::string spv_path;               //必要
    std::string entry_name;             //必要
    vk::ShaderStageFlagBits stage_flag;    //ここはAPIが埋めるので記入しなくていい
    std::unordered_map<std::string, uint32_t> color_attachment_location_map; //fragment shader でしか使わない   
                                                
public:
    ShaderInfo(std::string _spv_path, std::string _entry): spv_path (_spv_path), entry_name(_entry), stage_flag(vk::ShaderStageFlagBits()){;}
    void print();
    int get_color_attachment_location(std::string name);
};

//------------------
//struct : pipeline shader resource info
//------------------
//一つのパイプラインに対応する
struct PipelineShaderResourceInfo{
public:
    std::vector<ShaderInfo> shader_infos;
    DescsetsInfo local_descsets_info;

public:
    PipelineShaderResourceInfo& add_shader_info(std::string spv_path, std::string entry_name);
    void print();
};


//------------------
//struct : pipeline registry
//------------------
struct PipelineRegistry{
public:
    std::unordered_map<std::string, PipelineShaderResourceInfo> core;

public:
    PipelineShaderResourceInfo& operator[](std::string pipeline_name);
    void register_pipeline_name(std::string pipeline_name);
    bool has_pipeline_name(std::string pipeline_name);
    void print();
    void print(std::string pipeline_name);
};

//-----auto binding methods
//spvfileの読み取り
std::vector<uint32_t> read_spv_file(std::string& file_name);

//一つのspv fileからdescset layoutを読み出す
void auto_bind_from_spv_file(std::string& pipeline_name, DescsetsInfo* _descsets_info, ShaderInfo& shader_info);

//一つのpipelineを構成する複数のspv fileからdescset layoutを構築する
DescsetsInfo auto_bind_from_spv_files(std::string& pipeline_name, std::vector<ShaderInfo>& shader_infos);

//entry nameからstageを読み出す
void fill_pipeline_stage_from_entry_name(spirv_cross::Compiler& compiler, ShaderInfo* shader_info);

//
DescsetsInfo extract_global_and_build_local_descsets_info(PipelineRegistry& pipeline_registry);

//spv と vkの型変換
spv::ExecutionModel vk_shader_stage_to_spv_exection_model(vk::ShaderStageFlags vk_stage_flag);
vk::ShaderStageFlagBits spv_exection_model_to_vk_shader_stage(spv::ExecutionModel spv_stage);


}