#pragma once

// render instance への参照
struct InstanceReference{
    uint32_t        render_instance_id_base;
    uint32_t        render_instance_id;
    float32_t4x4    model_matrix;
    float32_t3x3    reverse_transpose_3x3_model_matrix; //instanceの分も含む全体の逆変換行列
};