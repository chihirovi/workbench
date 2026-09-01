#pragma once

struct VertexType0 {
    float3      position;
    //float16_t   normal[3];
    uint16_t    encode_normal;
    //float16_t   tangent[4];
    uint16_t    encode_tangent[2];
    //float16_t   tex_coord[2];
    uint8_t     padding0;
    uint8_t     padding1;
    uint8_t     color[4];
    uint32_t    packed_tex_coord;
};
