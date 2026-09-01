#pragma once

enum MaterialBlendMode : uint32_t{
    opaque  = 0,
    blend   = 1,
    mask    = 2,
};

struct Material {
    uint32_t    is_double_side;
    uint32_t    blend_mode;
    float       alpha_cut_off;

    float       base_color_factor[4];
    float       emissive_factor[4];
    float       metallic_factor;
    float       roughness_factor;
    float       normal_scale;
    float       occlusion_strength;

    uint32_t    texture_id_base;
    uint32_t    texture_view_id_base;

    int32_t     texture_view_id_albedo;          //-1がtextureなし，default textureは用意しないので，shaderで分岐
    int32_t     texture_view_id_normal;
    int32_t     texture_view_id_metallic_roughness;
    int32_t     texture_view_id_occlusion;
    int32_t     texture_view_id_emissive;

    uint32_t    is_pbrSpecularGlossiness_material;
    float       specular_factor[3];
    uint32_t    is_transmission;
};