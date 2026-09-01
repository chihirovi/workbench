#include "BLAS.hpp"

namespace Irori::RayTrace{
    
UniqueAS create_blas(Core::VulkanContext* vc, std::vector<BlasSubMeshInfo>& blas_create_info){
    
    // transform matrix 
    std::vector<vk::TransformMatrixKHR> matrix_data;
    matrix_data.resize(blas_create_info.size());

    for(auto [index, sub_mesh_info] : blas_create_info|std::views::enumerate){
        matrix_data[index] = convert_transform(sub_mesh_info.transform);
    }

    Core::UniqueBuffer matrix_buffer = Core::UniqueBuffer::common(
        vc,
        blas_create_info.size()*sizeof(vk::TransformMatrixKHR),
        false,
        vk::BufferUsageFlagBits::eShaderDeviceAddress|
        vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR|
        vk::BufferUsageFlagBits::eTransferDst,
        64
    );
    matrix_buffer.transfer_data(matrix_data.data(), blas_create_info.size()*sizeof(vk::TransformMatrixKHR));

    //geometorys info
    std::vector<vk::AccelerationStructureGeometryKHR> geometrys;
    std::vector<uint32_t> primitive_counts;
    
    for(auto [index, sub_mesh_info] : blas_create_info|std::views::enumerate){

        //create geometry info
        auto triangles = vk::AccelerationStructureGeometryTrianglesDataKHR()
            .setVertexStride(sub_mesh_info.vertex_stride)
            .setMaxVertex(sub_mesh_info.vertex_num)
            .setVertexFormat(sub_mesh_info.vertex_format)
            .setVertexData(sub_mesh_info.vertex_address)
            .setIndexType(sub_mesh_info.index_type)
            .setIndexData(sub_mesh_info.index_address)
            .setTransformData(matrix_buffer.get_device_address() + index*sizeof(vk::TransformMatrixKHR)); 
                                           
        //1 geometry have some triangles
        auto geometry = vk::AccelerationStructureGeometryKHR()
            .setGeometryType(vk::GeometryTypeKHR::eTriangles)
            .setGeometry(triangles)
            .setFlags(sub_mesh_info.geometry_frag);//any hit によって設定を変える
        
        uint32_t primitive_count = sub_mesh_info.index_num/3;
        
        geometrys.push_back(geometry);
        primitive_counts.push_back(primitive_count);
    }
    
    //create + build BLAS
    return std::move(
        UniqueAS(
            vc,
            vk::AccelerationStructureTypeKHR::eBottomLevel,
            geometrys,
            primitive_counts
        )
    );
}
    
}