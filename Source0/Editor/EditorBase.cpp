#include "EditorBase.hpp"

namespace Irori::Editor{

UniqueEditorBase::UniqueEditorBase(BOOST_RV_REF(UniqueEditorBase) rhs){
    *this = std::move(rhs);
}

UniqueEditorBase& UniqueEditorBase::operator=(BOOST_RV_REF(UniqueEditorBase) rhs){
    this->~UniqueEditorBase();
    this->vulkan_context = rhs.vulkan_context;
    this->m_context = rhs.m_context;
    this->imgui_descriptorpool = std::move(rhs.imgui_descriptorpool);
    this->imgui_renderpass = std::move(rhs.imgui_renderpass);
    this->framebuffer = std::move(rhs.framebuffer);
    this->render_image= std::move(rhs.render_image);
    rhs.m_context = nullptr;
    rhs.vulkan_context = nullptr;
    
    return *this;
}

UniqueEditorBase::UniqueEditorBase(Core::VulkanContext* vc, Core::UniqueSwapchain& swapchain){
    m_context = ImGui::CreateContext();
    vulkan_context = vc;
    ImGui::SetCurrentContext(m_context);
    ImGui::StyleColorsDark();
    
    ImGui_ImplGlfw_InitForVulkan(vc->window->window_handler, true);
    
    /*descriptor pool*/
    std::vector<vk::DescriptorPoolSize> poolSize = {
        {vk::DescriptorType::eSampler,100},
        {vk::DescriptorType::eCombinedImageSampler,100},
        {vk::DescriptorType::eSampledImage,100},
        {vk::DescriptorType::eStorageImage,100},
        {vk::DescriptorType::eUniformTexelBuffer,100},
        {vk::DescriptorType::eStorageTexelBuffer,100},
        {vk::DescriptorType::eUniformBuffer,100},
        {vk::DescriptorType::eStorageBuffer,100},
        {vk::DescriptorType::eUniformBufferDynamic,100},
        {vk::DescriptorType::eStorageBufferDynamic,100},
        {vk::DescriptorType::eInputAttachment,100}
    };

    vk::DescriptorPoolCreateInfo poolInfo = {};
    poolInfo.setFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet);
    poolInfo.setMaxSets(100*static_cast<uint32_t>(poolSize.size()));
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSize.size());
    poolInfo.pPoolSizes = poolSize.data();

    imgui_descriptorpool = vc->device->createDescriptorPoolUnique(poolInfo);

    // renderpass
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = (VkFormat)vc->surface_format.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; 
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    imgui_renderpass = vc->device->createRenderPassUnique(renderPassInfo);
    
    // image + frame buffer
    render_image = Core::UniqueImage::Builder2D(vulkan_context)
        .set_size_format_usage_layout(
            vulkan_context->window->w(), vulkan_context->window->h(),
            vk::Format::eR8G8B8A8Unorm,
            vk::ImageUsageFlagBits::eTransferSrc|vk::ImageUsageFlagBits::eColorAttachment|vk::ImageUsageFlagBits::eTransferDst,
            vk::ImageLayout::eColorAttachmentOptimal
        )
        .build();

    vk::ImageView frameBufAttachments[1];
    frameBufAttachments[0] = render_image.image_view.get();

    vk::FramebufferCreateInfo frameBufCreateInfo;
    frameBufCreateInfo.width = render_image.image_create_info.extent.width;
    frameBufCreateInfo.height = render_image.image_create_info.extent.height;
    frameBufCreateInfo.layers = 1;
    frameBufCreateInfo.renderPass = *imgui_renderpass;
    frameBufCreateInfo.attachmentCount = 1;
    frameBufCreateInfo.pAttachments = frameBufAttachments;

    framebuffer = vulkan_context->device->createFramebufferUnique(frameBufCreateInfo);

    // 
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = *vc->instance;
    init_info.PhysicalDevice = vc->physical_device;
    init_info.Device = *vc->device;
    init_info.QueueFamily = vc->queue_family_index;
    init_info.Queue = vc->queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = *imgui_descriptorpool;
    init_info.Allocator = nullptr;
    init_info.ImageCount = swapchain.swapchain_images.size();
    init_info.MinImageCount = swapchain.swapchain_images.size();
    init_info.PipelineInfoMain.RenderPass = *imgui_renderpass; 
    init_info.PipelineInfoMain.Subpass = 0; 
    init_info.CheckVkResultFn = nullptr;
    init_info.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    
    ImGui_ImplVulkan_Init(&init_info);
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    io.FontGlobalScale = 1.2f; 
}

void UniqueEditorBase::cmd_render_(vk::CommandBuffer& cmd_buf, std::function<void(void)> make_ui){
    ImGui::SetCurrentContext(m_context);
    ImGui_ImplGlfw_NewFrame();
    ImGui_ImplVulkan_NewFrame();
    ImGui::NewFrame();
    
    make_ui();

    vk::Rect2D rect({0,0}, {render_image.image_create_info.extent.width, render_image.image_create_info.extent.height});
    vk::ClearColorValue tc;
    tc.setFloat32({0.0, 0.0, 0.0, 1.0});
    vk::ClearValue clear_color;
    clear_color.setColor(tc);

    vk::RenderPassBeginInfo renderPassInfo{};
    renderPassInfo.setRenderPass(*imgui_renderpass);
    renderPassInfo.setFramebuffer(*framebuffer); 
    renderPassInfo.setRenderArea(rect);
    renderPassInfo.setClearValues({clear_color});

    cmd_buf.beginRenderPass(renderPassInfo, vk::SubpassContents::eInline);

    ImGui::Render();
    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd_buf);

    cmd_buf.endRenderPass();     
}

void UniqueEditorBase::destroy(){
    ImGui::SetCurrentContext(m_context);
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext(); 
}

UniqueEditorBase::~UniqueEditorBase(){
    if(m_context == nullptr) return;
    destroy();
}

}