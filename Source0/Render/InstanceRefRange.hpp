#pragma once
#include <cstdint>

namespace Irori::Render{
    
struct InstanceRefRange{
    uint32_t instance_ref_base;
    uint32_t instance_ref_count;
}__attribute__((packed, aligned(1)));

    static_assert(sizeof(InstanceRefRange) == 8, "InstanceRefRange must be 8 bytes!");
}