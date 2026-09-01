#pragma once

struct WA_Entry{
    uint32_t index;
    float32_t threshold;
};
    
struct LightSample{
    float32_t3 position = 0.0;
    float32_t3 normal = 0.0;
    float32_t3 emittanse = 0.0;
    float32_t  inv_pdf = 0.0;

    __init() {
        position = float3(0.0);
        normal = float3(1.0, 0.0, 0.0);
        emittanse = float3(0.0);
        inv_pdf = 0.0f;
    }
};

struct LightPrimitive{
    float32_t inv_pdf; // = 1 / [A_prim / sum(A_prim)];
};

struct LightPrimitiveWaTableRange{
    uint32_t base;
    uint32_t count;
};

struct LightObject{
    LightPrimitiveWaTableRange primitive_wa_table_range;
    uint32_t render_instance_base;
    uint32_t render_instance_index;
    float32_t4x4 scene_model_transform; // instance の変換行列を含まない, = scene::model_inst.transform
    float32_t surface_total_area; // local の変換のみ適応した面積
    float32_t inv_pdf; // = 1 / [ A_obj * Intensity / sum(A_obj * Intensity) ];
};

struct LightSampleDesc {
    uint32_t    light_object_num;
    uint32_t    padding0;
    WA_Entry*   light_object_pdf_wa_table;
    LightObject* light_object_table;
    WA_Entry*   light_primitive_pdf_wa_table;
};
