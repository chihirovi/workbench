#include "../Include/GlobalDescriptor.h.slang"
#include "../Include/ColorSpace.h.slang"

#include "./Include/payload.h.slang"

[shader("amplification")] //task shaderのこと
[numthreads(1, 1, 1)]
void main_task(uint3 global_thread_id : SV_DispatchThreadID) {
    
    // early return
    InstanceRefRange instance_ref_range = frame_render_data.common_mask_instance_ref_range;
    if(global_thread_id.x >= instance_ref_range.instance_ref_count) return;
    
    // get instance reference
    InstanceReference instance_ref = frame_render_data.instance_reference_table[instance_ref_range.instance_ref_base + global_thread_id.x];
    
    // get instance 
    task_2_mesh_payload.global_instance_idx = instance_ref.render_instance_id_base + instance_ref.render_instance_id;
    Instance instance = frame_render_data.instance_table[task_2_mesh_payload.global_instance_idx];
    
    // dispatch mesh
    task_2_mesh_payload.global_material_idx = uint(int(instance.material_id_base) + int(instance.material_id));

    task_2_mesh_payload.raw_vertex_addr     = uint64_t(instance.raw_vertex_addr);
    task_2_mesh_payload.raw_index_addr      = uint64_t(instance.raw_index_addr);
    task_2_mesh_payload.raw_triangle_num    = instance.raw_triangle_num;
    task_2_mesh_payload.model_matrix        = instance_ref.model_matrix;
    task_2_mesh_payload.reverse_transpose_3x3_model_matrix = instance_ref.reverse_transpose_3x3_model_matrix;
    
    // material base color factor
    Material *material = &frame_render_data.material_table[task_2_mesh_payload.global_material_idx];
    float4 material_bcf = float4(material.base_color_factor[0], material.base_color_factor[1], material.base_color_factor[2], material.base_color_factor[3]);
    material_bcf.xyz = srgb_to_linear(material_bcf.xyz);
    task_2_mesh_payload.material_base_color_factor = material_bcf;

    uint32_t triangle_num = instance.raw_triangle_num;
    DispatchMesh((triangle_num + MAX_TRIANGLE - 1) / MAX_TRIANGLE, 1, 1, task_2_mesh_payload);        
}