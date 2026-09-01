#include "AS.hpp"


namespace Irori::RayTrace{
    
vk::PhysicalDeviceAccelerationStructurePropertiesKHR UniqueAS::getAsProp(Core::VulkanContext* vc){
    auto features2 = vc->physical_device
        .getProperties2<vk::PhysicalDeviceProperties2,
        vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();

    auto as_prop = features2.get<vk::PhysicalDeviceAccelerationStructurePropertiesKHR>();       
    
    return as_prop; 
}

vk::DeviceAddress UniqueAS::get_AS_address(){
    ASSERT_WITH_MSG((this->as_address != 0), "this AS is null handle, can't get AS address.");
    return as_address;
}

UniqueAS::UniqueAS(
    Core::VulkanContext* vc, 
    vk::AccelerationStructureTypeKHR type,
    std::vector<vk::AccelerationStructureGeometryKHR>& geometrys,
    std::vector<uint32_t>& primitive_counts){
    
       
    //create build info
    vk::BuildAccelerationStructureFlagsKHR flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    if(type == vk::AccelerationStructureTypeKHR::eTopLevel) flags |= vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate; 
    auto geometry_build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(type)
        .setMode(vk::BuildAccelerationStructureModeKHR::eBuild)
        .setFlags(flags)
        .setGeometries(geometrys);
    
    auto build_sizes = vc->device->getAccelerationStructureBuildSizesKHR(
        vk::AccelerationStructureBuildTypeKHR::eDevice, 
        geometry_build_info, 
        primitive_counts
    );
    
    //create buffer for AS
    buffer_as = Core::UniqueBuffer::common(
        vc,
        build_sizes.accelerationStructureSize,
        false,
        vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR|vk::BufferUsageFlagBits::eShaderDeviceAddress,
        8
    );

    //create AS
    accel_struct = vc->device->createAccelerationStructureKHRUnique(
        vk::AccelerationStructureCreateInfoKHR()
            .setBuffer(buffer_as.buffer) 
            .setSize(build_sizes.accelerationStructureSize)
            .setType(type)
    );

    //create scratch buffer (to build AS)
    auto as_prop = getAsProp(vc);

    Core::UniqueBuffer scratch_buffer = Core::UniqueBuffer::common(
        vc,
        build_sizes.buildScratchSize,
        false,
        vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress, 
        as_prop.minAccelerationStructureScratchOffsetAlignment
    ); 


    if(type == vk::AccelerationStructureTypeKHR::eTopLevel){
        scratch_buffer_update = Core::UniqueBuffer::common(
            vc,
            build_sizes.updateScratchSize,
            false,
            vk::BufferUsageFlagBits::eStorageBuffer|vk::BufferUsageFlagBits::eShaderDeviceAddress, 
            as_prop.minAccelerationStructureScratchOffsetAlignment
        );
    }

    //build 
    geometry_build_info 
        .setDstAccelerationStructure(*accel_struct)
        .setScratchData(scratch_buffer.get_device_address());
    
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> build_range_infos; 
                           
    for(auto pc: primitive_counts){
        build_range_infos.push_back(
            vk::AccelerationStructureBuildRangeInfoKHR()
                .setPrimitiveCount(pc) 
                .setPrimitiveOffset(0)
                .setFirstVertex(0)
                .setTransformOffset(0)
        );
    }
    
    Core::Submit::one_time(vc, [&](vk::CommandBuffer& cb){
        //one geometry build info
        cb.buildAccelerationStructuresKHR(
            geometry_build_info, 
            build_range_infos.data()
        );                
    });
    
    //get address
    as_address = vc->device->getAccelerationStructureAddressKHR(
        vk::AccelerationStructureDeviceAddressInfoKHR()
            .setAccelerationStructure(*accel_struct)
    ); 
}

UniqueAS::UniqueAS(BOOST_RV_REF(UniqueAS) rhs){
    *this = std::move(rhs);
}

UniqueAS& UniqueAS::operator=(BOOST_RV_REF(UniqueAS) rhs){
    this->accel_struct = std::move(rhs.accel_struct);
    this->buffer_as = std::move(rhs.buffer_as);
    this->scratch_buffer_update = std::move(rhs.scratch_buffer_update);
    this->as_address = rhs.as_address;
    rhs.as_address = 0;
    return *this;
}

/*utility*/
vk::TransformMatrixKHR convert_transform(glm::mat4x4& m){
    vk::TransformMatrixKHR mtx;
    auto tmp = glm::transpose(m);
    memcpy(&mtx.matrix[0], &tmp[0], sizeof(float) * 4);
    memcpy(&mtx.matrix[1], &tmp[1], sizeof(float) * 4);
    memcpy(&mtx.matrix[2], &tmp[2], sizeof(float) * 4);
    
    return mtx;
} 

}