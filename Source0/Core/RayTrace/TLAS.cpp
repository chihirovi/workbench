#include "TLAS.hpp"

namespace Irori::RayTrace{

UniqueAS create_tlas(
    Core::VulkanContext* vc, 
    std::vector<TlasInstanceInfo>& tlas_create_info,
    std::vector<UniqueAS>& bottom_ASs){
    
    
    //instances info
    std::vector<vk::AccelerationStructureInstanceKHR> accel_instances;
    
    //create instance info
    for(auto& tlas_inst_info : tlas_create_info){

        vk::TransformMatrixKHR transform = convert_transform(tlas_inst_info.transform); 

        auto accel_instance = vk::AccelerationStructureInstanceKHR()
            .setTransform(transform)
            .setInstanceCustomIndex(tlas_inst_info.instance_custom_index)
            .setMask(0xFFFFFFFF)
            .setFlags(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable)
            .setAccelerationStructureReference(bottom_ASs[tlas_inst_info.blas_index].get_AS_address())
            //ここでメッシュの参照hit_groupを変える, 0で固定しとく，じゃないと設計が複雑になりすぎる
            .setInstanceShaderBindingTableRecordOffset(0);

        accel_instances.push_back(accel_instance);
    }

    //instance data buffer (on GPU)
    //uniqueBufferVma instances_buffer(
    auto tlas_instances_data_buffer = Core::UniqueBuffer::common(
        vc,
        accel_instances.size()*sizeof(vk::AccelerationStructureInstanceKHR),
        true,
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|vk::BufferUsageFlagBits::eShaderDeviceAddress,
        16
    );
    tlas_instances_data_buffer.transfer_data(accel_instances.data(), accel_instances.size()*sizeof(vk::AccelerationStructureInstanceKHR));

    

    auto tlas_instances_data = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(vk::False)   
        .setData(tlas_instances_data_buffer.get_device_address());

    //If type is VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR, 
    //geometryCount must be 1 
    std::vector<vk::AccelerationStructureGeometryKHR> tlas_geometrys;
    tlas_geometrys.push_back(vk::AccelerationStructureGeometryKHR()); //must be 1
    tlas_geometrys[0]
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry(tlas_instances_data)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);//any hitを呼ぶならここは設定なし

    std::vector<uint32_t> primitive_counts(1);//must be 1
    primitive_counts[0] = tlas_create_info.size();

    //create + build TLAS
    return std::move(UniqueAS(
        vc,
        vk::AccelerationStructureTypeKHR::eTopLevel,
        tlas_geometrys, //geometoryは必ず1つ
        primitive_counts//geometoryに含まれるinstance(BLAS)の数
    ));
}

void cmd_update_tlas(
    vk::CommandBuffer& cmd_buf,
    UniqueAS& tlas_AS,
    uint32_t tlas_instance_num,
    Core::UniqueBuffer& tlas_instances_data_buffer){

    auto tlas_instances_data = vk::AccelerationStructureGeometryInstancesDataKHR()
        .setArrayOfPointers(vk::False)   
        .setData(tlas_instances_data_buffer.get_device_address());
    
    
    std::vector<vk::AccelerationStructureGeometryKHR> tlas_geometrys;
    tlas_geometrys.push_back(vk::AccelerationStructureGeometryKHR()); //must be 1
    tlas_geometrys[0]
        .setGeometryType(vk::GeometryTypeKHR::eInstances)
        .setGeometry(tlas_instances_data)
        .setFlags(vk::GeometryFlagBitsKHR::eOpaque);//any hitを呼ぶならここは設定なし

    //geometry info
    auto geometry_build_info = vk::AccelerationStructureBuildGeometryInfoKHR()
        .setType(vk::AccelerationStructureTypeKHR::eTopLevel)
        .setMode(vk::BuildAccelerationStructureModeKHR::eUpdate)
        .setFlags(vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace|vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate)
        .setGeometries(tlas_geometrys)
        .setDstAccelerationStructure(*tlas_AS.accel_struct)
        .setSrcAccelerationStructure(*tlas_AS.accel_struct)
        .setScratchData(tlas_AS.scratch_buffer_update.get_device_address());

    //range
    std::vector<vk::AccelerationStructureBuildRangeInfoKHR> build_range_infos(1); 
    build_range_infos[0]
        .setPrimitiveCount(tlas_instance_num) 
        .setPrimitiveOffset(0)
        .setFirstVertex(0)
        .setTransformOffset(0);

    // update
    cmd_buf.buildAccelerationStructuresKHR(
        geometry_build_info,
        build_range_infos.data()
    );
    
}

}