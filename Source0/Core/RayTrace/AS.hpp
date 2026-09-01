#pragma once
#include "../Common/VulkanContext.hpp"
#include "../Common/Buffer.hpp"
#include "../Common/Submit.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Irori::RayTrace{
    
class UniqueAS{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueAS);
    vk::DeviceAddress  as_address = 0;

public:
    vk::UniqueAccelerationStructureKHR accel_struct;
    Core::UniqueBuffer buffer_as;
    Core::UniqueBuffer scratch_buffer_update;

public:
    UniqueAS(BOOST_RV_REF(UniqueAS) rhs);               //move constractor
    UniqueAS& operator=(BOOST_RV_REF(UniqueAS) rhs);    //move assignment
    
    UniqueAS(){;}
    UniqueAS(
        Core::VulkanContext* vc, 
        vk::AccelerationStructureTypeKHR type,
        std::vector<vk::AccelerationStructureGeometryKHR>& geometrys,
        std::vector<uint32_t>& primitive_counts
    );
    //primitive_counts: BLAS->num of triangles par geometry, 
    //primitive_counts: TLAS->num of BLAS(instance) par geometry
    //
    vk::PhysicalDeviceAccelerationStructurePropertiesKHR getAsProp(Core::VulkanContext* vc);
    vk::DeviceAddress get_AS_address();
};
    
/*utility*/
vk::TransformMatrixKHR convert_transform(glm::mat4x4& m); 

}