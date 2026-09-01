#pragma once
#include "AS.hpp"
#include <ranges>

namespace Irori::RayTrace{
    
struct BlasSubMeshInfo{
    vk::DeviceAddress vertex_address;
    vk::DeviceAddress index_address;
    uint32_t          vertex_stride;
    uint32_t          vertex_num;
    uint32_t          index_num;
    vk::Format        vertex_format;
    vk::IndexType     index_type;
    glm::f32mat4x4    transform;
    vk::GeometryFlagBitsKHR geometry_frag;
};

UniqueAS create_blas(Core::VulkanContext* vc, std::vector<BlasSubMeshInfo>& blas_create_info);
    
}