#pragma once

struct AreaLightSampler {
    RWStructuredBuffer<float> light_triangle;

    __init(RWStructuredBuffer<float> buffer) {
        light_triangle = buffer;
    }
    
    void sample(float3 rand, out float3 pos, out float pdf, out float3 normal, out float3 Le){
        uint32_t triangle_num = uint32_t(light_triangle[0]);

        uint32_t triangle_idx = uint32_t(rand[0] * triangle_num);        
        
        uint32_t base = triangle_idx*3+4;
        float3 vertex0 = float3(light_triangle[base + 0], light_triangle[base + 1], light_triangle[base + 2]);        
        float3 vertex1 = float3(light_triangle[base + 3], light_triangle[base + 4], light_triangle[base + 5]);        
        float3 vertex2 = float3(light_triangle[base + 6], light_triangle[base + 7], light_triangle[base + 8]);

        float su = sqrt(rand[1]);
        float b = 1.0 - su;
        float c = rand[2] * su;
        float a = 1.0 - b - c;
        pos = a * vertex0 + b * vertex1 + c * vertex2;
        
        float3 v01 = (vertex1 - vertex0);
        float3 v02 = (vertex2 - vertex0);
        float area = 0.5f*length(cross(v01, v02));
        pdf = 1.0f/area * 1.0f/(float)triangle_num;
        
        normal = normalize(cross(v01, v02));
        Le = float3(light_triangle[1], light_triangle[2], light_triangle[3]);
        /*
        float3 v0 = float3(-0.200000, 1.980000, -0.200000);
        float3 v1 = float3(0.200000, 1.980000, -0.200000);
        float3 v2 = float3(0.200000, 1.980000, 0.200000);
        float3 v3 = float3(-0.200000, 1.980000, 0.200000);
        
        pos = v0 + rand[0]*(v1-v0) * rand[1]*(v3-v0);
        normal = float3(0.0, -1.0, 0.0);
        pdf = 1.0;
        Le = float3(light_triangle[1], light_triangle[2], light_triangle[3]);
        */
    }
};