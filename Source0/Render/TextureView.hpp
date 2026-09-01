#pragma once
#include <cstdint>

namespace Irori::Render{
    
    struct TextureView{
        uint32_t texture_id;
        uint32_t sampler_type;
    }__attribute__((packed, aligned(1)));

    static_assert(sizeof(TextureView) == 8, "TextureView must be 8 bytes!");
}