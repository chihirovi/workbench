#include "Submit.hpp"

namespace Irori::Core::Submit{

void one_time(
    VulkanContext* vc,
    std::function<void(vk::CommandBuffer&)> inst){

    //create cmd buffer
    auto tmp_cmd_bufs = vc->device->allocateCommandBuffersUnique(
        vk::CommandBufferAllocateInfo()
            .setCommandPool(*vc->cmd_pool)
            .setCommandBufferCount(1)
            .setLevel(vk::CommandBufferLevel::ePrimary)
    );

    //recode to cmd buffer
    tmp_cmd_bufs[0]->begin(
        vk::CommandBufferBeginInfo()
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
    ); 
    inst(*tmp_cmd_bufs[0]);
    tmp_cmd_bufs[0]->end();

    //submit cmd buffer
    auto submit_info = vk::SubmitInfo()
        .setCommandBufferCount(1)
        .setCommandBuffers(*tmp_cmd_bufs[0]);

    vc->queue.submit({submit_info});
    vc->queue.waitIdle();
}

void cmd_submit_binary_semaphore(
    VulkanContext* vc,
    vk::CommandBuffer command_buffer, 
    vk::PipelineStageFlags wait_stage,
    vk::Semaphore wait_semaphore, 
    vk::Semaphore signal_semaphore, 
    vk::Fence signal_fence){
    
    auto submit_info = vk::SubmitInfo()
        .setCommandBuffers(command_buffer)
        .setWaitDstStageMask(wait_stage)
        .setWaitSemaphores(wait_semaphore)
        .setSignalSemaphores(signal_semaphore);

    vc->queue.submit({submit_info}, signal_fence);
}


}