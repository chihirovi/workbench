#pragma once

#include "../Core/Common/VulkanContext.hpp"
#include "../Core/Common/Swapchain.hpp"
#include "../Core/Common/Image.hpp"

#include <dearImgui/imgui.h>
#include <dearImgui/imgui_impl_glfw.h>
#include <dearImgui/imgui_impl_vulkan.h>
#include <dearImgui/imgui_internal.h>
#include <boost_1_88_0/boost/move/move.hpp>
#include <vulkan/vulkan.hpp>
#include <functional>

namespace Irori::Editor{

class UniqueEditorBase{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueEditorBase);

public:
    ImGuiContext* m_context = nullptr;
    Core::VulkanContext* vulkan_context = nullptr;
    vk::UniqueDescriptorPool imgui_descriptorpool;
    vk::UniqueRenderPass imgui_renderpass;
    vk::UniqueFramebuffer framebuffer;
    Core::UniqueImage render_image;

public:
    UniqueEditorBase(BOOST_RV_REF(UniqueEditorBase) rhs);               //move constractor
    UniqueEditorBase& operator=(BOOST_RV_REF(UniqueEditorBase) rhs);    //move assignment
                                                                                                        
    UniqueEditorBase(){}
    UniqueEditorBase(Core::VulkanContext* vc, Core::UniqueSwapchain& swapchain);
    void cmd_render_(vk::CommandBuffer& cmd_buf, std::function<void(void)> make_ui);
    void destroy();
    virtual ~UniqueEditorBase();
};
    
}