#pragma once
#include "VulkanContext.hpp"

namespace Irori::Core::Submit{

void one_time(VulkanContext* vc, std::function<void(vk::CommandBuffer&)> inst);

void cmd_submit_binary_semaphore(VulkanContext* vc, vk::CommandBuffer command_buffer, vk::PipelineStageFlags wait_stage, vk::Semaphore wait_semaphore, vk::Semaphore signal_semaphore, vk::Fence signal_fence);

}