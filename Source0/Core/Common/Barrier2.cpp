#include "Barrier2.hpp"
#include "Submit.hpp"

//cmd image barrier
namespace Irori::Core::CmdImageBarrier{

void barrier( 
    vk::CommandBuffer& cb,
    vk::AccessFlagBits2 src_access_mask,
    vk::AccessFlagBits2 dst_access_mask,
    vk::PipelineStageFlagBits2 src_stage,
    vk::PipelineStageFlagBits2 dst_stage,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::Image image,
    vk::ImageSubresourceRange subresource_range){
    
    vk::ImageMemoryBarrier2 image_barrier2 = vk::ImageMemoryBarrier2()
        .setSrcAccessMask(src_access_mask)
        .setDstAccessMask(dst_access_mask)
        .setSrcStageMask(src_stage)
        .setDstStageMask(dst_stage)
        .setOldLayout(old_layout)
        .setNewLayout(new_layout)
        .setImage(image)
        .setSubresourceRange(subresource_range);
    
    vk::DependencyInfo dep_info = vk::DependencyInfo()
        .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
        .setImageMemoryBarriers(image_barrier2);

    cb.pipelineBarrier2(dep_info);
}

void barrier( 
    vk::CommandBuffer& cb,
    vk::AccessFlagBits2 src_access_mask,
    vk::AccessFlagBits2 dst_access_mask,
    vk::PipelineStageFlagBits2 src_stage,
    vk::PipelineStageFlagBits2 dst_stage,
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    UniqueImage& image){
    barrier(
        cb, 
        src_access_mask, dst_access_mask, 
        src_stage, dst_stage, 
        old_layout, new_layout, 
        image.get_raw_image(), 
        image.image_view_create_info.subresourceRange
    );
    image.set_current_mip0_array0_layout(new_layout);
}

void comp_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,          //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,           //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader, //src stage
        vk::PipelineStageFlagBits2::eComputeShader, //dst stage
        vk::ImageLayout::eGeneral,                  //old layout
        vk::ImageLayout::eGeneral,                  //new layout
        image
    );
}

void top_pipe_2_transfer_Psrc2Tdst(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eNone,                     //src access mask
        vk::AccessFlagBits2::eTransferWrite,            //dst access masc
        vk::PipelineStageFlagBits2::eTopOfPipe,         //src stage
        vk::PipelineStageFlagBits2::eTransfer,          //dst stage
        vk::ImageLayout::ePresentSrcKHR,                //old layout
        vk::ImageLayout::eTransferDstOptimal,           //new layout
        image
    );   
}

void transfer_2_bottom_pipe_Tdst2Psrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eTransferWrite,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eTransfer,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eTransferDstOptimal,           //old layout
        vk::ImageLayout::ePresentSrcKHR,                //new layout
        image
    );   
}


void frag_CA_write_2_transfer_CAopt2Tsrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eColorAttachmentWrite,            //src access mask
        vk::AccessFlagBits2::eTransferRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          //src stage
        vk::PipelineStageFlagBits2::eTransfer,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::eTransferSrcOptimal,                //new layout
        image
    );   
}

void transfer_2_bottom_pipe_Tsrc2CAopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eTransferRead,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eTransfer,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eTransferSrcOptimal,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}


void top_pipe_2_comp_write_Psrc2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eNone,                     //src access mask
        vk::AccessFlagBits2::eShaderStorageWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eTopOfPipe,          //src stage
        vk::PipelineStageFlagBits2::eComputeShader,      //dst stage
        vk::ImageLayout::ePresentSrcKHR,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}
void comp_write_2_bottom_pipe_G2Psrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::ePresentSrcKHR,                //new layout
        image
    );      
}


void top_pipe_2_frag_CA_write_Psrc2CAopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eNone,            //src access mask
        vk::AccessFlagBits2::eColorAttachmentWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eTopOfPipe,          //src stage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,      //dst stage
        vk::ImageLayout::ePresentSrcKHR,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}

void frag_CA_write_2_bottom_pipe_CAopt2Psrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eColorAttachmentWrite,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::ePresentSrcKHR,                //new layout
        image
    );    
}

void frag_CA_write_2_comp_read_CAopt2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eColorAttachmentWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          //src stage
        vk::PipelineStageFlagBits2::eComputeShader,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void frag_CA_write_2_rgen_read_CAopt2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eColorAttachmentWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,          //src stage
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void comp_read_2_bottom_pipe_G2CAopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageRead,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}

void comp_write_2_transfer_G2Tsrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eTransferRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eTransfer,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eTransferSrcOptimal,                //new layout
        image
    );   
}

void transfer_2_bottom_pipe_Tsrc2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eTransferRead,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eTransfer,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eTransferSrcOptimal,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void rgen_write_2_transfer_G2Tsrc(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eTransferRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,          //src stage
        vk::PipelineStageFlagBits2::eTransfer,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eTransferSrcOptimal,                //new layout
        image
    );   
}

void rgen_sample_2_comp_read_SRopt2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderSampledRead,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,          //src stage
        vk::PipelineStageFlagBits2::eComputeShader,      //dst stage
        vk::ImageLayout::eShaderReadOnlyOptimal,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );      
}


void comp_write_2_rgen_sample_G2SRopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderSampledRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eShaderReadOnlyOptimal,                //new layout
        image
    );         
}

void rgen_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,          //src stage
        vk::PipelineStageFlagBits2::eComputeShader,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void comp_write_2_frag_CA_write_G2CAopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eColorAttachmentWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}

void comp_write_2_rgen_write(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}


void rgen_write_2_frag_CA_read_G2sample(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderSampledRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eRayTracingShaderKHR,          //src stage
        vk::PipelineStageFlagBits2::eFragmentShader,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eShaderReadOnlyOptimal,                //new layout
        image
    );   
}

void frag_CA_write_2_frag_CA_read_CAopt2sample(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eColorAttachmentWrite,            //src access mask
        vk::AccessFlagBits2::eShaderSampledRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eFragmentShader,          //src stage
        vk::PipelineStageFlagBits2::eFragmentShader,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::eShaderReadOnlyOptimal,                //new layout
        image
    );      
}

void comp_write_2_frag_CA_read_G2sample(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderSampledRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eFragmentShader,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eShaderReadOnlyOptimal,                //new layout
        image
    );
}

void frag_read_2_bottom_pipe_Sample2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderSampledRead,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eFragmentShader,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eShaderReadOnlyOptimal,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void transfer_2_bottom_pipe_Tdst2fragCA(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eTransferRead,            //src access mask
        vk::AccessFlagBits2::eNone,                     //dst access masc
        vk::PipelineStageFlagBits2::eTransfer,          //src stage
        vk::PipelineStageFlagBits2::eBottomOfPipe,      //dst stage
        vk::ImageLayout::eTransferSrcOptimal,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}

void top_pipe_2_transfer_CAopt2Tdst(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eNone,            //src access mask
        vk::AccessFlagBits2::eTransferWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eTopOfPipe,          //src stage
        vk::PipelineStageFlagBits2::eTransfer,      //dst stage
        vk::ImageLayout::eColorAttachmentOptimal,           //old layout
        vk::ImageLayout::eTransferDstOptimal,                //new layout
        image
    );   
}

void transfer_2_frag_write_CA_Tdst2CAopt(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eTransferWrite,            //src access mask
        vk::AccessFlagBits2::eColorAttachmentWrite,                     //dst access masc
        vk::PipelineStageFlagBits2::eTransfer,          //src stage
        vk::PipelineStageFlagBits2::eColorAttachmentOutput,      //dst stage
        vk::ImageLayout::eTransferDstOptimal,           //old layout
        vk::ImageLayout::eColorAttachmentOptimal,                //new layout
        image
    );   
}

void comp_write_2_frag_read_G2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eComputeShader,          //src stage
        vk::PipelineStageFlagBits2::eFragmentShader,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void frag_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eShaderStorageWrite,            //src access mask
        vk::AccessFlagBits2::eShaderStorageRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eFragmentShader,          //src stage
        vk::PipelineStageFlagBits2::eComputeShader,      //dst stage
        vk::ImageLayout::eGeneral,           //old layout
        vk::ImageLayout::eGeneral,                //new layout
        image
    );   
}

void frag_CA_write_2_frag_CA_read_D2D(vk::CommandBuffer& cb, UniqueImage& image){
    barrier(
        cb,
        vk::AccessFlagBits2::eDepthStencilAttachmentWrite,           //src access mask
        vk::AccessFlagBits2::eDepthStencilAttachmentRead,                     //dst access masc
        vk::PipelineStageFlagBits2::eLateFragmentTests,          //src stage
        vk::PipelineStageFlagBits2::eEarlyFragmentTests,      //dst stage
        vk::ImageLayout::eDepthStencilAttachmentOptimal,           //old layout
        vk::ImageLayout::eDepthStencilAttachmentOptimal,                //new layout
        image
    );   
}

}


//image barrier one time
namespace Irori::Core::ImageBarrier{

//convert image 
void convert_image_layout(
    VulkanContext* vulkan_context, 
    vk::Image image, 
    vk::ImageLayout old_layout,
    vk::ImageLayout new_layout,
    vk::ImageSubresourceRange subresource_range){

    Submit::one_time(vulkan_context, [&](vk::CommandBuffer& cb){
        CmdImageBarrier::barrier(
            cb,
            vk::AccessFlagBits2::eNone,                 //src access mask
            vk::AccessFlagBits2::eNone,                 //dst access masc
            vk::PipelineStageFlagBits2::eAllCommands,   //src stage
            vk::PipelineStageFlagBits2::eAllCommands,   //dst stage
            old_layout,                                 //old layout
            new_layout,                                 //new layout
            image,
            subresource_range
        );       
    });
}

void convert_image_layout(
    VulkanContext* vulkan_context,
    UniqueImage& image,
    vk::ImageLayout new_layout){
    convert_image_layout(
        vulkan_context, 
        image.get_raw_image(), 
        image.current_mip0_array0_layout,
        new_layout, 
        image.image_view_create_info.subresourceRange
    );
    image.set_current_mip0_array0_layout(new_layout);
}

}



//cmd buffer barrier
namespace Irori::Core::CmdBufferBarrier{
    
    
void comp_write_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);
}

void comp_write_2_indirect_arg(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eDrawIndirect) //compute の indirect dispatchでも行けるらしい
        .setDstAccessMask(vk::AccessFlagBits2::eIndirectCommandRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);
}
    
void transfer_dst_2_task_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eTaskShaderEXT) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);
}

void comp_write_2_vert_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eVertexShader) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);
}

void transfer_dst_2_as_update_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eAccelerationStructureBuildKHR) 
        .setDstAccessMask(vk::AccessFlagBits2::eAccelerationStructureReadKHR)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);   
}

void transfer_dst_2_raygen_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info); 
}

void transfer_dst_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eTransfer)
        .setSrcAccessMask(vk::AccessFlagBits2::eTransferWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info); 
}

void rgen_write_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);    
}

void rgen_read_2_comp_write(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setDstStageMask(vk::PipelineStageFlagBits2::eComputeShader) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);    
}

void comp_write_2_rgen_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);   
}

void comp_write_2_chit_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eRayTracingShaderKHR) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);      
}

void comp_write_2_mesh_read(vk::CommandBuffer& cb, UniqueBuffer& buffer){
    vk::BufferMemoryBarrier2 buffer_barrier = vk::BufferMemoryBarrier2()
        .setSrcStageMask(vk::PipelineStageFlagBits2::eComputeShader)
        .setSrcAccessMask(vk::AccessFlagBits2::eShaderStorageWrite)
        .setDstStageMask(vk::PipelineStageFlagBits2::eMeshShaderEXT) 
        .setDstAccessMask(vk::AccessFlagBits2::eShaderStorageRead)
        .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
        .setBuffer(buffer.get_raw_buffer())
        .setOffset(0)
        .setSize(vk::WholeSize);

    vk::DependencyInfo dependency_info = vk::DependencyInfo()
        .setBufferMemoryBarriers(buffer_barrier);
    
    cb.pipelineBarrier2(dependency_info);         
}

}