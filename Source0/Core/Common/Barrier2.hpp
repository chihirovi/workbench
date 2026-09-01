#pragma once
#include "VulkanContext.hpp"
#include "Image.hpp"
#include "Buffer.hpp"


// image barrier with command
namespace Irori::Core::CmdImageBarrier{

void barrier(vk::CommandBuffer& cb, vk::AccessFlagBits2 src_access_mask, vk::AccessFlagBits2 dst_accessmask, vk::PipelineStageFlagBits2 src_stage, vk::PipelineStageFlagBits2 dst_stage, vk::ImageLayout old_layout, vk::ImageLayout new_layout, UniqueImage& image);
void barrier(vk::CommandBuffer& cb, vk::AccessFlagBits2 src_access_mask, vk::AccessFlagBits2 dst_access_mask, vk::PipelineStageFlagBits2 src_stage, vk::PipelineStageFlagBits2 dst_stage, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::Image image, vk::ImageSubresourceRange subresource_range);

// stage 2 stage, layout 2 layout の命名
void comp_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image);

void top_pipe_2_transfer_Psrc2Tdst(vk::CommandBuffer& cb, UniqueImage& image);
void transfer_2_bottom_pipe_Tdst2Psrc(vk::CommandBuffer& cb, UniqueImage& image);

void frag_CA_write_2_transfer_CAopt2Tsrc(vk::CommandBuffer& cb, UniqueImage& image);
void transfer_2_bottom_pipe_Tsrc2CAopt(vk::CommandBuffer& cb, UniqueImage& image);

void top_pipe_2_comp_write_Psrc2G(vk::CommandBuffer& cb, UniqueImage& image);
void comp_write_2_bottom_pipe_G2Psrc(vk::CommandBuffer& cb, UniqueImage& image);

void top_pipe_2_frag_CA_write_Psrc2CAopt(vk::CommandBuffer& cb, UniqueImage& image);
void frag_CA_write_2_bottom_pipe_CAopt2Psrc(vk::CommandBuffer& cb, UniqueImage& image);

void frag_CA_write_2_comp_read_CAopt2G(vk::CommandBuffer& cb, UniqueImage& image);
void comp_read_2_bottom_pipe_G2CAopt(vk::CommandBuffer& cb, UniqueImage& image);

void comp_write_2_transfer_G2Tsrc(vk::CommandBuffer& cb, UniqueImage& image);
void transfer_2_bottom_pipe_Tsrc2G(vk::CommandBuffer& cb, UniqueImage& image);

void rgen_write_2_transfer_G2Tsrc(vk::CommandBuffer& cb, UniqueImage& image);


void rgen_sample_2_comp_read_SRopt2G(vk::CommandBuffer& cb, UniqueImage& image);

void comp_write_2_rgen_sample_G2SRopt(vk::CommandBuffer& cb, UniqueImage& image);

void rgen_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image);

void frag_CA_write_2_rgen_read_CAopt2G(vk::CommandBuffer& cb, UniqueImage& image);

void comp_write_2_frag_CA_write_G2CAopt(vk::CommandBuffer& cb, UniqueImage& image);

void comp_write_2_rgen_write(vk::CommandBuffer& cb, UniqueImage& image);

void rgen_write_2_frag_CA_read_G2sample(vk::CommandBuffer& cb, UniqueImage& image);
void frag_CA_write_2_frag_CA_read_CAopt2sample(vk::CommandBuffer& cb, UniqueImage& image);
void comp_write_2_frag_CA_read_G2sample(vk::CommandBuffer& cb, UniqueImage& image);

void frag_read_2_bottom_pipe_Sample2G(vk::CommandBuffer& cb, UniqueImage& image);
void transfer_2_bottom_pipe_Tdst2fragCA(vk::CommandBuffer& cb, UniqueImage& image);

void top_pipe_2_transfer_CAopt2Tdst(vk::CommandBuffer& cb, UniqueImage& image);

void transfer_2_frag_write_CA_Tdst2CAopt(vk::CommandBuffer& cb, UniqueImage& image);


void comp_write_2_frag_read_G2G(vk::CommandBuffer& cb, UniqueImage& image);

void frag_write_2_comp_read_G2G(vk::CommandBuffer& cb, UniqueImage& image);

void frag_CA_write_2_frag_CA_read_D2D(vk::CommandBuffer& cb, UniqueImage& image);

}


//convert image 
namespace Irori::Core::ImageBarrier{

void convert_image_layout(VulkanContext* vulkan_context, UniqueImage& image, vk::ImageLayout new_layout);
void convert_image_layout(VulkanContext* vulkan_context, vk::Image image, vk::ImageLayout old_layout, vk::ImageLayout new_layout, vk::ImageSubresourceRange subresource_range);

}


//buffer barrier with command
namespace Irori::Core::CmdBufferBarrier{
    
void comp_write_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void comp_write_2_indirect_arg(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void transfer_dst_2_task_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void transfer_dst_2_as_update_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void transfer_dst_2_raygen_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void transfer_dst_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void rgen_write_2_comp_read(vk::CommandBuffer& cb, UniqueBuffer& buffer); 
void rgen_read_2_comp_write(vk::CommandBuffer& cb, UniqueBuffer& buffer); 
void comp_write_2_rgen_read(vk::CommandBuffer& cb, UniqueBuffer& buffer); 
void comp_write_2_vert_read(vk::CommandBuffer& cb, UniqueBuffer& buffer); 
void comp_write_2_chit_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
void comp_write_2_mesh_read(vk::CommandBuffer& cb, UniqueBuffer& buffer);
}