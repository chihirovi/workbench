#define SPV_REFLECT_IMPLEMENTATION
#include "ShaderReflect.hpp"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <vector>

#define COMP_DESC_BIND_INFO_WITHOUT_STAGE(X_, Y_, PNAME_, SET_, BIND_, DESCTYPE_)                                                                                                                                                                                                                        \
    do{                                                                                                                                                                                                                                             \
        ASSERT_WITH_MSG((X_.name == Y_.name), std::format("Pipeline [{}] : {} descriptor set [{}] bind [{}] name doesn't match , {}, {}", PNAME_, DESCTYPE_, SET_, BIND_, X_.name, Y_.name));                                                         \
        ASSERT_WITH_MSG((X_.type == Y_.type), std::format("Pipeline [{}] : {} descriptor set [{}] bind [{}] type doesn't match , {}, {}", PNAME_, DESCTYPE_, SET_, BIND_, vk::to_string(X_.type), vk::to_string(Y_.type)));                           \
        ASSERT_WITH_MSG((X_.descriptor_count == Y_.descriptor_count), std::format("Pipeline [{}] : {} descriptor set [{}] bind [{}] desc count doesn't match , {}, {}", PNAME_, DESCTYPE_, SET_, BIND_, X_.descriptor_count, Y_.descriptor_count));   \
    }while(0)

namespace Irori::Core::ShaderReflect{

//------------------
//struct : Descsets Info
//------------------
std::unordered_map<uint32_t, DescriptorBindingInfo>& DescsetsInfo::operator[](uint32_t set_num){
    return core[set_num];
}

bool DescsetsInfo::is_empty_set_bind(uint32_t set, uint32_t bind){
    if(core.contains(set)){
        if(core[set].contains(bind)){
            return false;
        }
    }
    return true;
}

bool DescsetsInfo::is_empty_set(uint32_t set){
    if(core.contains(set)) return false;
    return true;
}

void DescsetsInfo::print(){
    printf("------------------------\n");
    for(auto& [set_num, desc_set] : core){
        for(auto& [bind_num, set_bind] : desc_set){
            printf("set : %d, binding : %d \n", set_num, bind_num);
            printf("    name : %s\n", set_bind.name.c_str());
            printf("    type : %s\n", vk::to_string(set_bind.type).c_str());
            printf("    array size : %d\n", set_bind.descriptor_count);
            printf("    stage : %s\n", vk::to_string(set_bind.stage_frags).c_str());
            printf("\n");
        }
    }    
}

//------------------
//struct : shader info
//------------------
void ShaderInfo::print(){
    printf("spv path    : %s\n", spv_path.c_str());
    printf("entry name  : %s\n", entry_name.c_str());
    printf("stage       : %s\n", vk::to_string(stage_flag).c_str());
    if(stage_flag != vk::ShaderStageFlagBits::eFragment) return;
    printf("output location names : \n");
    for(auto& [name, location] : color_attachment_location_map) printf("   %s, %d\n", name.c_str(), location);
    printf("\n");
}

int ShaderInfo::get_color_attachment_location(std::string name){
    ASSERT_WITH_MSG((stage_flag == vk::ShaderStageFlagBits::eFragment), std::format("this shader is [{}], you can only use this method at eFragment shader", vk::to_string(stage_flag)));
    ASSERT_WITH_MSG((color_attachment_location_map.contains(name)), std::format("this shader dosen't contains output color attachment name [{}]", name));
    return color_attachment_location_map[name];
}
//------------------
//struct : pipeline shader resource info
//------------------
PipelineShaderResourceInfo& PipelineShaderResourceInfo::add_shader_info(
    std::string spv_path, 
    std::string entry_name){
    shader_infos.push_back(ShaderInfo(spv_path, entry_name));
    return *this;
}

void PipelineShaderResourceInfo::print(){
    for(auto& shader_info : shader_infos){
        shader_info.print();
        printf("\n");
    }
    local_descsets_info.print();
    printf("\n");
}

//------------------
//struct : pipeline registry
//------------------
PipelineShaderResourceInfo& PipelineRegistry::operator[](std::string pipeline_name){
    ASSERT_WITH_MSG(has_pipeline_name(pipeline_name), std::format("this registry doesn't has pipeline name [{}], please use register() to enable it", pipeline_name));
    return core[pipeline_name];
}

void  PipelineRegistry::register_pipeline_name(std::string pipeline_name){
    core[pipeline_name];
}

bool PipelineRegistry::has_pipeline_name(std::string pipeline_name){
    return core.contains(pipeline_name);
}

void PipelineRegistry::print(){
    for(auto& [name , _] : core){
        print(name);
    }   
}

void PipelineRegistry::print(std::string pipeline_name){
    printf("----------------------------\n");
    printf("pipeline name : %s\n", pipeline_name.c_str());
    (*this)[pipeline_name].print();
    printf("\n");       
}

//------------------
//auto binding methods
//------------------
std::vector<uint32_t> read_spv_file(std::string& file_name){
    /*check file existing*/
    ASSERT_WITH_MSG(std::filesystem::exists(file_name), std::format("file doesn't exist : {}", file_name));
    
    /*read file to u8 buffer*/
    size_t spvfile_size = std::filesystem::file_size(file_name);
    std::ifstream spv_file(file_name, std::ios_base::binary);
    std::vector<char> spvfile_data(spvfile_size);
    spv_file.read(spvfile_data.data(), spvfile_size);
    
    /*copy file to u32 buffer*/
    auto u32_file_size = spvfile_size%4==0 ? spvfile_size/4 : spvfile_size/4+1;

    std::vector<uint32_t> spvfile_data_u32(u32_file_size);

    std::memcpy(spvfile_data_u32.data(), spvfile_data.data(), spvfile_size);
   
    return std::move(spvfile_data_u32);
}

void auto_bind_from_spv_file(std::string& pipeline_name, DescsetsInfo* _descsets_info, ShaderInfo& shader_info){

    DescsetsInfo& descsets_info = *_descsets_info;

    auto code = ShaderReflect::read_spv_file(shader_info.spv_path);
    
    spirv_cross::Compiler spv_reflection(code);
    
    //check entry name + stage
    ShaderReflect::fill_pipeline_stage_from_entry_name(spv_reflection, &shader_info);
    spv_reflection.set_entry_point(shader_info.entry_name, ShaderReflect::vk_shader_stage_to_spv_exection_model(shader_info.stage_flag));
    
    auto register_desc_set_bind = [&](
        spirv_cross::SmallVector<spirv_cross::Resource>& resources,
        vk::DescriptorType desc_type){

        for(auto& resource : resources){
            uint32_t set_num = spv_reflection.get_decoration(resource.id, spv::DecorationDescriptorSet);
            uint32_t bind_num = spv_reflection.get_decoration(resource.id, spv::DecorationBinding);
            std::string name = spv_reflection.get_name(resource.id);
            
            ASSERT_WITH_MSG((set_num <= 7), std::format("Pipeline {} : Irori only supports a maximum of 8 sets, this set num is {}", pipeline_name, set_num));
            
            spirv_cross::SPIRType type = spv_reflection.get_type(resource.type_id);
            uint32_t array_size = 1;
            //通常の配列か，bindless配列かで切り替え，bindlessはサイスがわからないので最大値でお茶を濁す
            if(!type.array.empty()){
                uint32_t count = type.array[0];
                array_size = (count==0 ? IRORI_BINDLESS_DESC_MAX_SIZE : count);
            }
            
            //登録，未登録ならそのまま，
            //登録済みなら，一致を確認してから，stageを付け加える
            DescriptorBindingInfo desc_bind_info;
            desc_bind_info.name = name;
            desc_bind_info.type = desc_type;
            desc_bind_info.descriptor_count = array_size;
            desc_bind_info.stage_frags = shader_info.stage_flag;

            if(descsets_info.is_empty_set_bind(set_num, bind_num)){
                descsets_info[set_num][bind_num] = desc_bind_info;

            }else{
                auto& target = descsets_info[set_num][bind_num];
                
                COMP_DESC_BIND_INFO_WITHOUT_STAGE(target, desc_bind_info, pipeline_name, set_num, bind_num, "local");
                
                target.stage_frags |= desc_bind_info.stage_frags;
            }
        }
    };

    spirv_cross::ShaderResources shader_resources = spv_reflection.get_shader_resources();
    register_desc_set_bind(shader_resources.acceleration_structures,    vk::DescriptorType::eAccelerationStructureKHR);
    register_desc_set_bind(shader_resources.uniform_buffers,            vk::DescriptorType::eUniformBuffer);
    register_desc_set_bind(shader_resources.storage_buffers,            vk::DescriptorType::eStorageBuffer);
    register_desc_set_bind(shader_resources.storage_images,             vk::DescriptorType::eStorageImage);
    register_desc_set_bind(shader_resources.sampled_images,             vk::DescriptorType::eCombinedImageSampler);
    register_desc_set_bind(shader_resources.separate_images,            vk::DescriptorType::eSampledImage);
    register_desc_set_bind(shader_resources.separate_samplers,          vk::DescriptorType::eSampler);

    /*
     *  struct FragOut2{
            float4 out00 : SV_Target;
            float4 out11 : SV_Target;
        };

        struct FragOut{
            float4 out0 : SV_Target;
            float4 out1 : SV_Target;
            FragOut2 out3;
        };
        のような場合，out3.out00が変数名として記録される
    */
    if(shader_info.stage_flag == vk::ShaderStageFlagBits::eFragment){
        std::vector<uint32_t> output_nums;
        for(auto& output : shader_resources.stage_outputs){
            uint32_t output_num = spv_reflection.get_decoration(output.id, spv::DecorationLocation);
            output_nums.push_back(output_num);
            std::string name = spv_reflection.get_name(output.id);
            auto pos = name.find('.');
            std::string suffix;
            if (pos != std::string::npos) {
                suffix = name.substr(pos + 1);
            }
            shader_info.color_attachment_location_map[suffix] = output_num;
        }
    }
}


DescsetsInfo auto_bind_from_spv_files(std::string& pipeline_name, std::vector<ShaderInfo>& shader_infos){
    DescsetsInfo descsets_layout_info;
    for(auto& shader_info : shader_infos){
        ShaderReflect::auto_bind_from_spv_file(pipeline_name, &descsets_layout_info, shader_info);
    }
    return std::move(descsets_layout_info);
}


//utility
spv::ExecutionModel vk_shader_stage_to_spv_exection_model(vk::ShaderStageFlags vk_stage_flag){
    if(vk_stage_flag == vk::ShaderStageFlagBits::eCompute)          return spv::ExecutionModelGLCompute;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eVertex)           return spv::ExecutionModelVertex;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eFragment)         return spv::ExecutionModelFragment;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eTaskEXT)          return spv::ExecutionModelTaskEXT;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eMeshEXT)          return spv::ExecutionModelMeshEXT;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eRaygenKHR)        return spv::ExecutionModelRayGenerationKHR;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eMissKHR)          return spv::ExecutionModelMissKHR;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eClosestHitKHR)    return spv::ExecutionModelClosestHitKHR;
    if(vk_stage_flag == vk::ShaderStageFlagBits::eAnyHitKHR)        return spv::ExecutionModelAnyHitKHR;

    ASSERT_WITH_MSG(false, std::format("doesn't support this shader stage : {}", vk::to_string(vk_stage_flag)));
}

vk::ShaderStageFlagBits spv_exection_model_to_vk_shader_stage(spv::ExecutionModel spv_stage){
    if(spv_stage == spv::ExecutionModelGLCompute)           return vk::ShaderStageFlagBits::eCompute;
    if(spv_stage == spv::ExecutionModelVertex)              return vk::ShaderStageFlagBits::eVertex;
    if(spv_stage == spv::ExecutionModelFragment)            return vk::ShaderStageFlagBits::eFragment;
    if(spv_stage == spv::ExecutionModelTaskEXT)             return vk::ShaderStageFlagBits::eTaskEXT;
    if(spv_stage == spv::ExecutionModelMeshEXT)             return vk::ShaderStageFlagBits::eMeshEXT;
    if(spv_stage == spv::ExecutionModelRayGenerationKHR)    return vk::ShaderStageFlagBits::eRaygenKHR;
    if(spv_stage == spv::ExecutionModelMissKHR)             return vk::ShaderStageFlagBits::eMissKHR;
    if(spv_stage == spv::ExecutionModelClosestHitKHR)       return vk::ShaderStageFlagBits::eClosestHitKHR;
    if(spv_stage == spv::ExecutionModelAnyHitKHR)           return vk::ShaderStageFlagBits::eAnyHitKHR;

    ASSERT_WITH_MSG(false, std::format("doesn't support this shader stage : spv {}", std::to_string(spv_stage)));
}


//entry name から ステージを割り出す
//そして引数のshader_infoのstageを埋める
void fill_pipeline_stage_from_entry_name(
    spirv_cross::Compiler& compiler,
    ShaderInfo* shader_info){
    
    ASSERT_WITH_MSG(((shader_info->stage_flag) == vk::ShaderStageFlags()), std::format("entry name {} : input ShaderInfo.stage_frag must be empty", shader_info->entry_name));

    auto entry_points = compiler.get_entry_points_and_stages();

    bool name_found = false;
    for (const auto& ep : entry_points) {
        if (ep.name == shader_info->entry_name) {
            name_found = true;
            shader_info->stage_flag = ShaderReflect::spv_exection_model_to_vk_shader_stage(ep.execution_model);
            return; // OK
        }
    }

    printf("all entry point at [%s]\n", shader_info->spv_path.c_str());
    for (const auto& ep : entry_points) {
        if (ep.name == shader_info->entry_name) {
            printf("%s\n", ep.name.c_str());
        }
    }
    ASSERT_WITH_MSG(name_found, (shader_info->spv_path + " : Entry point not found: '" + shader_info->entry_name));
}

//pipelineのdesclayoutをマージして，過不足なく共通部分をくみ出す
//0,1,2,3はlocal descで，各パイプラインの固有のもの
//4,5,6,7はglobal descで，すべてのパイプラインで共通させる
//ここでは，globalの部分の切り分けと共通してるpipeline stageを調べる
DescsetsInfo extract_global_and_build_local_descsets_info(PipelineRegistry& pipeline_registry){

    DescsetsInfo global_descsets_info;

    for(auto& [pipeline_name , shader_resource_info] : pipeline_registry.core){

        auto temp_name = std::string(pipeline_name);
        auto temp_descsets_layout_info = ShaderReflect::auto_bind_from_spv_files(temp_name, shader_resource_info.shader_infos);
        
        for(int global_set_num : {4,5,6,7}){
            
            if(temp_descsets_layout_info.is_empty_set(global_set_num)) continue;
            
            for(auto& [binding_num, set_bind_info] : temp_descsets_layout_info[global_set_num]){

                if(global_descsets_info.is_empty_set_bind(global_set_num, binding_num)){
                    global_descsets_info[global_set_num][binding_num] = set_bind_info;

                }else{
                    auto& target = global_descsets_info[global_set_num][binding_num];
                    
                    COMP_DESC_BIND_INFO_WITHOUT_STAGE(target, set_bind_info, pipeline_name, global_set_num, binding_num, "global");
                    
                    //ここで使われているステージを数え上げている
                    target.stage_frags |= set_bind_info.stage_frags;
                }
            }
            
            //global部分のdesc setは消しておく，
            //そこは別に抜き出して外部で処理するので
            temp_descsets_layout_info.core.erase(global_set_num);
        }

        //local desc だけに処理したのを代入
        shader_resource_info.local_descsets_info = temp_descsets_layout_info;
    }
    
    return global_descsets_info;
}

}