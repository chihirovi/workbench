#pragma once
#include "../Common/VulkanContext.hpp"
#include "../Common/ShaderModule.hpp"
#include "../Common/DescriptorSets.hpp"
#include "../Common/Image.hpp"
#include <algorithm>
#include <ranges>

namespace Irori::Core{
    
struct ColorAttachmentInfo{
    std::string     name;
    UniqueImage*    image = nullptr;
    std::optional<vk::PipelineColorBlendAttachmentState> blend_state;
    std::optional<std::array<float, 4>>     clear_rgba;
};
    
class UniqueRasterPipelineTemplate{
private:
    BOOST_MOVABLE_BUT_NOT_COPYABLE(UniqueRasterPipelineTemplate);
    
public:
    VulkanContext* vulkan_context = nullptr;
    uint32_t width, height;

    // pipeline
    vk::UniquePipeline pipeline;
    vk::UniquePipelineLayout pipeline_layout;
    UniqueShaderModules shader_modules;
    UniqueDescriptorSets descriptor_sets;
    
    // frame buffer
    vk::UniqueFramebuffer framebuffer;
    
    // attachments
    std::vector<vk::ClearAttachment> clear_attachments;
    std::vector<vk::ClearRect> clear_rects;
    
    // renderpass
    vk::UniqueRenderPass renderpass;

public:
    UniqueRasterPipelineTemplate(){;}
    UniqueRasterPipelineTemplate(VulkanContext* vc){ vulkan_context = vc; };
    UniqueRasterPipelineTemplate(BOOST_RV_REF(UniqueRasterPipelineTemplate) rhs) = default;               //move constractor
    UniqueRasterPipelineTemplate& operator=(BOOST_RV_REF(UniqueRasterPipelineTemplate) rhs) = default;    //move assignment
    UniqueRasterPipelineTemplate& cmd_begin_renderpass(vk::CommandBuffer& cb);
    UniqueRasterPipelineTemplate& cmd_clear_attachments(vk::CommandBuffer& cb);
    UniqueRasterPipelineTemplate& cmd_bind_pipeline(vk::CommandBuffer& cb);
    void cmd_bind_local_descsets(vk::CommandBuffer& cb);

public:
    class Builder{
    public:
        VulkanContext* vc = nullptr;
        uint32_t w = 0, h = 0;
        
        // shader + descrioptor info
        ShaderReflect::PipelineShaderResourceInfo* pipeline_shader_resource_info = nullptr;
        ShaderReflect::DescsetsInfo* global_descsets_info = nullptr;
        
        // attachments info
        std::vector<std::string> color_attachment_names;
        std::vector<UniqueImage*> color_attachment_images;
        std::vector<std::optional<vk::PipelineColorBlendAttachmentState>> color_attachment_blend_states;
        std::vector<std::optional<vk::ClearColorValue>> color_attachment_clear_values;
        
        // depth attachment
        UniqueImage* depth_attachment_image;
        std::optional<float> clear_depth_value;

        // rasterizer info
        vk::PolygonMode polygon_mode;
        vk::CullModeFlagBits culling_mode;
        vk::FrontFace front_face;
        
        // depth buffer
        vk::Bool32 depth_test_enable = false;
        vk::Bool32 depth_write_enable = false;
        vk::CompareOp depth_compare_op;

        // vertex
        std::vector<vk::VertexInputBindingDescription>* vertex_input_descs = nullptr;
        std::vector<vk::VertexInputAttributeDescription>* input_attrib_descs = nullptr;

    public:
        Builder(VulkanContext* _vc){vc = _vc;} 
        Builder& set_w_h(uint32_t _w, uint32_t _h){w = _w, h = _h; return *this;}
        Builder& set_shader_desc_info(ShaderReflect::PipelineShaderResourceInfo& _a, ShaderReflect::DescsetsInfo& _b){pipeline_shader_resource_info = &_a; global_descsets_info = &_b; return *this;}
        Builder& add_color_attachment_info(ColorAttachmentInfo color_attachment_info);
        Builder& set_depth_test_state(bool _d_test_enable, bool _d_write_enable, vk::CompareOp _d_comp_op){depth_test_enable = _d_test_enable; depth_write_enable = _d_write_enable; depth_compare_op = _d_comp_op; return *this;};
        Builder& set_rasterizer_state(vk::PolygonMode pmode, vk::CullModeFlagBits cmoode, vk::FrontFace fface){polygon_mode = pmode; culling_mode = cmoode; front_face = fface; return *this;};
        Builder& set_vertex_input_info(std::vector<vk::VertexInputBindingDescription>* _a, std::vector<vk::VertexInputAttributeDescription>* _b){vertex_input_descs = _a; input_attrib_descs = _b; return *this;};
        Builder& set_depth_attachment_info(UniqueImage* _depth_image, std::optional<float> _clear_value){depth_attachment_image = _depth_image; clear_depth_value = _clear_value; return *this;}
        UniqueRasterPipelineTemplate build();
    };
}; 




}