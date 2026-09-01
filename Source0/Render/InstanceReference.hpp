#pragma once
#include <stdfloat>
#include <cstdint>
#include <stdfloat>
#include <type_traits>
#include <glm/glm.hpp>

namespace Irori::Render{

    // render instance への参照
    struct InstanceReference{
        uint32_t render_instance_id_base;
        uint32_t render_instance_id;
        glm::f32mat4x4  model_matrix; // instance の変換行列も含む，全体の変換行列， = scene::model_inst.transform * render_inst.tranform
        glm::f32mat3x3  reverse_transpose_3x3_model_matrix;
    };

    static_assert(sizeof(InstanceReference) == 108, "Instance Reference must be 108 bytes!");
} 