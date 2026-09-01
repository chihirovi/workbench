#pragma once

#define DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER 8
#define DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER 16
#define DDGI_PROBE_NUM_RAY 256
//#define DDGI_PROBE_NUM_RAY 192
//#define DDGI_PROBE_NUM_RAY 128
//#define DDGI_PROBE_NUM_RAY 64
#define DDGI_IRRADIANCE_ENCODE_GAMMA 1.0

struct DDGIDesc{
    float4      bottom_left_front_origin;      //-x, -y, -zの方の原点
    float4      scales;         //プローブの配置間隔
    uint32_t4   sizes;          //probe nums
    float4      probe_ray_rotation;  // xyzw
    float4      view_normal_bias;
    float4      min_front_face_dist_ray_backface_threshold;
};
