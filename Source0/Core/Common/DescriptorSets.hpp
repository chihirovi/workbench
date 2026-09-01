#pragma once
#include "VulkanContext.hpp"
#include "ShaderReflect.hpp"
#include "Buffer.hpp"
#include "Image.hpp"
#include "../RayTrace/TLAS.hpp"

namespace Irori::Core{

enum DescriptorScope : uint32_t{
    Global,
    Local,
};

class UniqueDescriptorSets{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueDescriptorSets);

public:
    //デストラクタは下から呼ばれるから，poolが先に破棄されないように，上に置いておく
    VulkanContext* vulkan_context = nullptr;
    vk::UniqueDescriptorPool local_desc_pool;
    std::vector<vk::UniqueDescriptorSetLayout> local_global_desc_set_layouts;
    std::unordered_map<uint32_t, vk::UniqueDescriptorSet> local_desc_sets;
    std::unordered_map<std::string, std::pair<uint32_t, uint32_t>> local_name_set_bind_map;
    ShaderReflect::DescsetsInfo local_descsets_info;
    DescriptorScope scope;
    
public:
    UniqueDescriptorSets();
    UniqueDescriptorSets(VulkanContext* _vc, ShaderReflect::DescsetsInfo& _local_descsets_info, ShaderReflect::DescsetsInfo& _global_descsets_info, DescriptorScope _scope);
    UniqueDescriptorSets(BOOST_RV_REF(UniqueDescriptorSets) rhs);               //move constractor
    UniqueDescriptorSets& operator=(BOOST_RV_REF(UniqueDescriptorSets) rhs);    //move assignment

    void create_local_global_descriptor_layouts(ShaderReflect::DescsetsInfo& global_descsets_info);
    vk::UniqueDescriptorSetLayout create_descriptor_layout(std::unordered_map<uint32_t, ShaderReflect::DescriptorBindingInfo>& binds_info); 
    void create_local_descriptor_pool();
    void create_local_descriptor_sets();
    void create_local_name_set_bind_map();
    vk::WriteDescriptorSet get_writer(std::string name);
    vk::WriteDescriptorSet get_writer(uint32_t set_num, uint32_t bind_num);
    UniqueDescriptorSets& update(std::string name, UniqueBuffer& buffer);
    UniqueDescriptorSets& update(std::string name, std::vector<UniqueBuffer*> buffers);
    UniqueDescriptorSets& update(std::string name, UniqueImage& image, vk::ImageLayout layout);
    UniqueDescriptorSets& update(std::string name, std::vector<vk::ImageView>& images, vk::ImageLayout layout);
    UniqueDescriptorSets& update(std::string name, std::vector<vk::Sampler>& samplers);
    UniqueDescriptorSets& update(std::string name, vk::UniqueAccelerationStructureKHR& top_as);
    UniqueDescriptorSets& UniqueDescriptorSets::update(std::string name, RayTrace::UniqueAS* tlas_s, int num);
    void bind_local_descriptor_sets(vk::CommandBuffer& cb, vk::PipelineLayout pipeline_layout, vk::PipelineBindPoint pipeline_bind_point);
    std::vector<vk::DescriptorSetLayout> get_descset_layouts();
                                                                                
    static UniqueDescriptorSets Local(VulkanContext* _vc, ShaderReflect::DescsetsInfo& _local_descsets_info, ShaderReflect::DescsetsInfo& _global_descsets_info);
    static UniqueDescriptorSets Global(VulkanContext* _vc, ShaderReflect::DescsetsInfo& _global_descsets_info);
};


}