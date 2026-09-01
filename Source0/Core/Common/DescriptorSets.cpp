#include "DescriptorSets.hpp"

namespace Irori::Core{

UniqueDescriptorSets::UniqueDescriptorSets(){;}

UniqueDescriptorSets::UniqueDescriptorSets(
    VulkanContext* _vc, 
    ShaderReflect::DescsetsInfo& _local_descsets_info, 
    ShaderReflect::DescsetsInfo& _global_descsets_info,
    DescriptorScope _scope){
    
    vulkan_context = _vc;
    scope = _scope;

    if(scope == DescriptorScope::Global){  //global descsetsかどうか判断する
        local_descsets_info = _global_descsets_info;   
    }else{
        local_descsets_info = _local_descsets_info;
    }
    
    // vulkanの仕様で，vec<desc set layout>の順番通りにset=0, 1と割り当てられるので，まずは空で初期化
    vk::DescriptorSetLayoutCreateInfo emptyLayoutInfo{};
    emptyLayoutInfo.bindingCount = 0;  // バインディング無し

    for (int i = 0; i < 8; i++) {
        local_global_desc_set_layouts.push_back(vulkan_context->device->createDescriptorSetLayoutUnique(emptyLayoutInfo));
    }
    
    create_local_name_set_bind_map();
    create_local_global_descriptor_layouts(_global_descsets_info);
    create_local_descriptor_pool();
    create_local_descriptor_sets();
}



void UniqueDescriptorSets::create_local_global_descriptor_layouts(ShaderReflect::DescsetsInfo& global_descsets_info){
    
    //create descset layout from local desc set build info
    for(auto& [local_set_num, binds_info] : local_descsets_info.core){

        if(scope == DescriptorScope::Global){
            ASSERT_WITH_MSG(((4<=local_set_num) && (local_set_num<= 7)), std::format("global desc set number must be 4, 5, 6, or 7, but this set number is {}", local_set_num));
        }else{
            ASSERT_WITH_MSG(((0<=local_set_num) && (local_set_num <= 3)), std::format("local desc set number must be 0, 1, 2, or 3, but this set number is {}", local_set_num));
        }

        local_global_desc_set_layouts[local_set_num] = create_descriptor_layout(binds_info);       
    }
    
    //create descset layout from global desc set build info
    if(scope == DescriptorScope::Global) return;
    for(auto& [global_set_num, binds_info] : global_descsets_info.core){

        ASSERT_WITH_MSG(((4<=global_set_num) && (global_set_num <= 7)), std::format("global desc set number must be 4, 5, 6, or 7, but this set number is {}", global_set_num));

        local_global_desc_set_layouts[global_set_num] = create_descriptor_layout(binds_info);       
    }
}


vk::UniqueDescriptorSetLayout UniqueDescriptorSets::create_descriptor_layout(
    std::unordered_map<uint32_t, ShaderReflect::DescriptorBindingInfo>& binds_info){

    // binding の数だけ bindingFlags を設定 
    std::vector<vk::DescriptorBindingFlags> binding_flags(
        binds_info.size(),
        vk::DescriptorBindingFlagBits::ePartiallyBound
    );
    
    // pNext に渡す構造体
    vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags_info{};
    binding_flags_info.bindingCount = static_cast<uint32_t>(binding_flags.size());
    binding_flags_info.pBindingFlags = binding_flags.data();
    
    //bindingsを作る
    std::vector<vk::DescriptorSetLayoutBinding> desc_set_layout_binding;
    for(auto& [bind_num, set_bind_info] : binds_info){
        vk::DescriptorSetLayoutBinding temp;
        temp.binding = bind_num;
        temp.descriptorType = set_bind_info.type;
        temp.descriptorCount = set_bind_info.descriptor_count;
        temp.stageFlags = set_bind_info.stage_frags;
        desc_set_layout_binding.push_back(temp);
    }

    auto tmp_uni_layout = vulkan_context->device->createDescriptorSetLayoutUnique(
        vk::DescriptorSetLayoutCreateInfo()
            .setBindings(desc_set_layout_binding)
            .setPNext(&binding_flags_info)
    );

    return std::move(tmp_uni_layout);
}


void UniqueDescriptorSets::create_local_descriptor_pool(){
    
    //まずは種類をカウント
    std::unordered_map<vk::DescriptorType, uint32_t> desc_type_count; 
    
    for(auto& [set_num, binds_info] : local_descsets_info.core){
        for(auto& [bind_num, bind_info] : binds_info){
            auto type = bind_info.type;
            auto count = bind_info.descriptor_count;            

            if(desc_type_count.contains(type)){
                desc_type_count[type] += count;
            }else{
                desc_type_count[type] = count;
            }
        }
    }
    
    //次に作る
    std::vector<vk::DescriptorPoolSize> desc_pool_sizes; 
    for(auto [type, count]: desc_type_count){
        desc_pool_sizes.push_back({type, count});
    }

    local_desc_pool = vulkan_context->device->createDescriptorPoolUnique(
        vk::DescriptorPoolCreateInfo()
            .setPoolSizes(desc_pool_sizes) 
            .setMaxSets((uint32_t)local_global_desc_set_layouts.size())
            .setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
    );
}
    
void UniqueDescriptorSets::create_local_descriptor_sets(){

    for(auto& [local_set_num, _] : local_descsets_info.core){

        auto local_desc_set = vulkan_context->device->allocateDescriptorSetsUnique(
            vk::DescriptorSetAllocateInfo()
                .setDescriptorPool(*local_desc_pool)
                .setSetLayouts(*local_global_desc_set_layouts[local_set_num])
        );
        
        local_desc_sets[local_set_num] = std::move(local_desc_set[0]);
    }
}


//move constractor
UniqueDescriptorSets::UniqueDescriptorSets(BOOST_RV_REF(UniqueDescriptorSets) rhs){
    *this = std::move(rhs);
}

//move assignment
UniqueDescriptorSets& UniqueDescriptorSets::operator=(BOOST_RV_REF(UniqueDescriptorSets) rhs){
    if(this == &rhs) return *this; 
    //moveの時は，poolが先に破棄されないように，後ろに持ってくる
    this->vulkan_context                    = rhs.vulkan_context;
    this->local_global_desc_set_layouts     = std::move(rhs.local_global_desc_set_layouts);
    this->local_desc_sets                   = std::move(rhs.local_desc_sets);
    this->local_desc_pool                   = std::move(rhs.local_desc_pool);
    this->local_name_set_bind_map           = rhs.local_name_set_bind_map;
    this->local_descsets_info               = rhs.local_descsets_info;
    this->scope                             = rhs.scope;
    rhs.vulkan_context = nullptr;
    return *this;
}

void UniqueDescriptorSets::create_local_name_set_bind_map(){
    for(auto& [set_num, binds_info] : local_descsets_info.core){
        for(auto& [bind_num, bind_info] : binds_info){
            local_name_set_bind_map[bind_info.name] = std::make_pair(set_num, bind_num);
        }
    }
}


vk::WriteDescriptorSet UniqueDescriptorSets::get_writer(std::string name){
    ASSERT_WITH_MSG(local_name_set_bind_map.contains(name), std::format("local_desc_sets doesn't contains [{}]", name));
    auto [set, bind] = local_name_set_bind_map[name];
    return get_writer(set, bind);
}

vk::WriteDescriptorSet UniqueDescriptorSets::get_writer(uint32_t set_num, uint32_t bind_num){
    if(local_descsets_info.is_empty_set_bind(set_num, bind_num)){
        ASSERT_WITH_MSG(false, std::format(" set = {}, bind = {}, is empty", set_num, bind_num));
    }

    ASSERT_WITH_MSG(local_desc_sets.contains(set_num), std::format("local_desc_sets doesn't contains set num (%d)", set_num));
    
    auto writer = vk::WriteDescriptorSet()
        .setDstSet(*local_desc_sets[set_num])
        .setDstBinding(bind_num)
        .setDescriptorCount(local_descsets_info[set_num][bind_num].descriptor_count)
        .setDescriptorType(local_descsets_info[set_num][bind_num].type);

    return writer;
}

//update methods
UniqueDescriptorSets& UniqueDescriptorSets::update(std::string name, UniqueBuffer& buffer){
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eUniformBuffer || writer.descriptorType == vk::DescriptorType::eStorageBuffer),
        std::format("[{}] type is [{}], but expect Uniform or Storage Buffer", name, vk::to_string(writer.descriptorType))        
    );
    
    ASSERT_WITH_MSG((buffer.get_raw_buffer() != VK_NULL_HANDLE), std::format("[{}] is NULL HANDLE", name));
    
    auto buffer_info = vk::DescriptorBufferInfo()
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setRange(buffer.buffer_size);
    
    writer
        .setBufferInfo(buffer_info)
        .setDescriptorCount(1);

    vulkan_context->device->updateDescriptorSets(writer, nullptr);
    return *this;
}

UniqueDescriptorSets& UniqueDescriptorSets::update(std::string name, std::vector<UniqueBuffer*> buffers){
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eUniformBuffer || writer.descriptorType == vk::DescriptorType::eStorageBuffer),
        std::format("[{}] type is [{}], but expect Uniform or Storage Buffer", name, vk::to_string(writer.descriptorType))        
    );
    
    for(auto& buffer: buffers){
        ASSERT_WITH_MSG((buffer->get_raw_buffer() != VK_NULL_HANDLE), std::format("[{}] is NULL HANDLE", name));
    }
    
    std::vector<vk::DescriptorBufferInfo> buffer_infos;
    for(auto& buffer: buffers){
        auto buffer_info = vk::DescriptorBufferInfo()
            .setBuffer(buffer->get_raw_buffer())
            .setOffset(0)
            .setRange(buffer->buffer_size);
        buffer_infos.push_back(buffer_info);
    }
    
    writer
        .setBufferInfo(buffer_infos)
        .setDescriptorCount(buffer_infos.size());

    vulkan_context->device->updateDescriptorSets(writer, nullptr);
    return *this;
}

UniqueDescriptorSets& UniqueDescriptorSets::update(
    std::string name, 
    UniqueImage& image, 
    vk::ImageLayout layout){

    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eStorageImage || writer.descriptorType == vk::DescriptorType::eSampledImage),
        std::format("[{}] type is [{}], but expect Storage Image or Sampled Image", name, vk::to_string(writer.descriptorType))        
    );
    
    ASSERT_WITH_MSG((image.get_raw_image() != VK_NULL_HANDLE), std::format("[{}] is NULL HANDLE", name));
    
    auto image_info = vk::DescriptorImageInfo()
        .setImageView(*image.image_view)
        .setImageLayout(layout);
    
    writer
        .setDescriptorCount(1)
        .setImageInfo(image_info);

    vulkan_context->device->updateDescriptorSets(writer, nullptr);
    return *this;
}

UniqueDescriptorSets& UniqueDescriptorSets::update(
    std::string name, 
    std::vector<vk::ImageView>& images, 
    vk::ImageLayout layout){
    
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eStorageImage || writer.descriptorType == vk::DescriptorType::eSampledImage),
        std::format("[{}] type is [{}], but expect Storage Image or Sampled Image", name, vk::to_string(writer.descriptorType))        
    );
    
    ASSERT_WITH_MSG((images.size() <= writer.descriptorCount), std::format("image count [{}] is larger than descriptor count [{}]", images.size(), writer.descriptorCount));

    std::vector<vk::DescriptorImageInfo> image_infos(images.size());
    for(int i=0; i<image_infos.size(); i++){
        image_infos[i]
            .setImageView(images[i])
            .setImageLayout(layout);
    }
    
    writer
        .setDstArrayElement(0) //配列の始まりのindex
        .setImageInfo(image_infos)
        .setDescriptorCount(image_infos.size());

    vulkan_context->device->updateDescriptorSets(writer, nullptr);

    return *this;
}


UniqueDescriptorSets& UniqueDescriptorSets::update(
    std::string name, 
    std::vector<vk::Sampler>& samplers){
 
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eSampler),
        std::format("[{}] type is [{}], but expect Sampler", name, vk::to_string(writer.descriptorType))        
    );
    
    ASSERT_WITH_MSG((samplers.size() <= writer.descriptorCount), std::format("image count [{}] is larger than descriptor count [{}]", samplers.size(), writer.descriptorCount));

    std::vector<vk::DescriptorImageInfo> sampler_infos(samplers.size());
    for(int i=0; i<sampler_infos.size(); i++){
        sampler_infos[i]
            .setSampler(samplers[i]);
    }
    
    writer
        .setDstArrayElement(0) //配列の始まりのindex
        .setImageInfo(sampler_infos)
        .setDescriptorCount(sampler_infos.size());

    vulkan_context->device->updateDescriptorSets(writer, nullptr);

    return *this;   
}

UniqueDescriptorSets& UniqueDescriptorSets::update(
    std::string name, 
    vk::UniqueAccelerationStructureKHR& top_as){
 
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eAccelerationStructureKHR),
        std::format("[{}] type is [{}], but expect tlas", name, vk::to_string(writer.descriptorType))        
    );
    
    vk::WriteDescriptorSetAccelerationStructureKHR as_info;
    as_info.setAccelerationStructures(*top_as);
    
    writer
        .setDstArrayElement(0) //配列の始まりのindex
        .setDescriptorCount(1)
        .setPNext(&as_info);

    vulkan_context->device->updateDescriptorSets(writer, nullptr);

    return *this;   
}

UniqueDescriptorSets& UniqueDescriptorSets::update(
    std::string name, 
    RayTrace::UniqueAS* tlas_s, int num){
 
    auto writer = get_writer(name);
    ASSERT_WITH_MSG(
        (writer.descriptorType == vk::DescriptorType::eAccelerationStructureKHR),
        std::format("[{}] type is [{}], but expect tlas", name, vk::to_string(writer.descriptorType))        
    );
    
    std::vector<vk::AccelerationStructureKHR> as_handles(num);
    for (int i = 0; i < num; i++)
        as_handles[i] = *(tlas_s[i].accel_struct);

    vk::WriteDescriptorSetAccelerationStructureKHR as_info{};
    as_info.setAccelerationStructureCount(num)
           .setPAccelerationStructures(as_handles.data());
    
    writer
        .setDstArrayElement(0) //配列の始まりのindex
        .setDescriptorCount(num)
        .setPNext(&as_info);

    vulkan_context->device->updateDescriptorSets(writer, nullptr);

    return *this;   
}

void UniqueDescriptorSets::bind_local_descriptor_sets(
    vk::CommandBuffer& cb,
    vk::PipelineLayout pipeline_layout, 
    vk::PipelineBindPoint pipeline_bind_point){
    
    for(auto& [set_num, _] : local_descsets_info.core){

        if(scope == DescriptorScope::Global){
            ASSERT_WITH_MSG(((4<=set_num) && (set_num<= 7)), std::format("global desc set number must be 4, 5, 6, or 7, but this set number is {}", set_num));
        }else{
            ASSERT_WITH_MSG(((0<=set_num) && (set_num<= 3)), std::format("local desc set number must be 0, 1, 2, or 3, but this set number is {}", set_num));
        }

        cb.bindDescriptorSets(
            pipeline_bind_point,
            pipeline_layout,
            set_num,                    //first set
            *local_desc_sets[set_num],  //temp,
            nullptr
        );
    }
}

std::vector<vk::DescriptorSetLayout> UniqueDescriptorSets::get_descset_layouts(){
    std::vector<vk::DescriptorSetLayout> temp;
    
    for(auto& d : local_global_desc_set_layouts){
        temp.push_back(*d);
    }
    return temp;
}


//static factory methosd
UniqueDescriptorSets UniqueDescriptorSets::Local(
    VulkanContext* _vc, 
    ShaderReflect::DescsetsInfo& _local_descsets_info, 
    ShaderReflect::DescsetsInfo& _global_descsets_info){
    
    return UniqueDescriptorSets(_vc, _local_descsets_info, _global_descsets_info, DescriptorScope::Local);
}

UniqueDescriptorSets UniqueDescriptorSets::Global(
    VulkanContext* _vc, 
    ShaderReflect::DescsetsInfo& _global_descsets_info){
    
    auto null_dummy = ShaderReflect::DescsetsInfo();
    return UniqueDescriptorSets(_vc, null_dummy, _global_descsets_info, DescriptorScope::Global);
}

}