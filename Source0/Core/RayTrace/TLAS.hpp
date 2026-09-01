#pragma once
#include "AS.hpp"

namespace Irori::RayTrace{

struct TlasInstanceInfo{
    uint32_t blas_index;
    glm::f32mat4x4 transform;
    uint32_t instance_custom_index;
};

//vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|vk::BufferUsageFlagBits::eShaderDeviceAddress,
UniqueAS create_tlas(
    Core::VulkanContext* vc, 
    std::vector<TlasInstanceInfo>& tlas_create_info,
    std::vector<UniqueAS>& bottom_ASs
);

void cmd_update_tlas(
    vk::CommandBuffer& cmd_buf,
    UniqueAS& tlas_AS,
    uint32_t tlas_instance_num,
    Core::UniqueBuffer& tlas_instances_data_buffer
);

}