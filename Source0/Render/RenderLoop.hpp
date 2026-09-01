#pragma once
#include "../Core/Common/VulkanContext.hpp"
#include "../Core/Common/Image.hpp"
#include "../Core/Common/Swapchain.hpp"
#include "../Core/Common/Submit.hpp"
#include <functional>
#include <chrono>

namespace Irori::Render{
    
// frame renderer : GPUで走るレンダリング処理
// cpu_async_task : submitした後は，CPUが暇なので，その隙間時間でCPU側で行う処理
void render_loop(
    Core::VulkanContext& vulkan_context, 
    Core::UniqueSwapchain& swapchain, 
    std::function<void(void)> cpu_task_before_rendering,
    std::function<void(vk::CommandBuffer&, Core::UniqueImage&)> frame_renderer,
    std::function<void(void)> cpu_async_task_in_rendering,
    std::function<void(float)> cpu_task_after_rendering
);
    
}