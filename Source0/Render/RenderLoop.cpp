#include "RenderLoop.hpp"

namespace Irori::Render{
    
void render_loop(
    Core::VulkanContext& vulkan_context, 
    Core::UniqueSwapchain& swapchain, 
    std::function<void(void)> cpu_task_before_rendering,
    std::function<void(vk::CommandBuffer&, Core::UniqueImage&)> frame_renderer,
    std::function<void(void)> cpu_async_task_in_rendering,
    std::function<void(float)> cpu_task_after_rendering){

    // sync objects
    // fence (sync cpu and gpu)
    auto submit_wait_fence = vulkan_context.device->createFenceUnique(
        vk::FenceCreateInfo()
    );

    //semaphore (sync gpu command and gpu command)
    auto next_img_ready_semaphore = vulkan_context.device->createSemaphoreUnique(
        vk::SemaphoreCreateInfo()
    );

    std::vector<vk::UniqueSemaphore> render_complete_semaphores; 
    for(int i=0; i<swapchain.swapchain_images.size(); i++){
        render_complete_semaphores.push_back(
            vulkan_context.device->createSemaphoreUnique(vk::SemaphoreCreateInfo())
        );
    }

    vulkan_context.window->loop([&](){
        //レンダリング前にやること
        cpu_task_before_rendering();
        
        // next image
        swapchain.acquire_next_image(*next_img_ready_semaphore);
        
        vk::UniqueCommandBuffer& cmd_buf = vulkan_context.cmd_bufs[0];
        
        // record command
        cmd_buf->begin(vk::CommandBufferBeginInfo());
        frame_renderer(*cmd_buf, swapchain.next_image());
        cmd_buf->end();
        
        // 計測開始
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // submit 
        Irori::Core::Submit::cmd_submit_binary_semaphore(
            &vulkan_context, 
            *cmd_buf, 
            vk::PipelineStageFlagBits::eAllCommands,
            *next_img_ready_semaphore, 
            *render_complete_semaphores[swapchain.next_image_index()], 
            *submit_wait_fence
        );
        
        swapchain.presentation(*render_complete_semaphores[swapchain.next_image_index()]);
        
        // GPU処理中はCPUが暇なので，次のフレームに必要な計算とかをここで
        cpu_async_task_in_rendering();
        
        // wait for rendering end
        vulkan_context.device->waitForFences(
            {*submit_wait_fence}, 
            vk::True, 
            UINT64_MAX
        );
        
        // 計測終了
        auto end_time = std::chrono::high_resolution_clock::now();

        // 経過時間をミリ秒単位で取得
        auto duration = std::chrono::duration<float, std::milli>(end_time - start_time);

        // renderingの完了後にやることにやること
        cpu_task_after_rendering(duration.count());

        // reset fence
        vulkan_context.device->resetFences({*submit_wait_fence});
    });
    
    vulkan_context.queue.waitIdle();
}

    
}