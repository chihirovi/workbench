#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


uint16_t encode_spherical_16(glm::f32vec3 nor){
    glm::f32vec2 v = glm::f32vec2(0.5 + 0.5 * atan2(nor.z, nor.x) / 3.141593, acos(nor.y) / 3.141593);
    glm::u32vec2 d = glm::u32vec2(glm::round(v * 255.0f));
    return d.x | (d.y << 8u);
}

glm::f32vec3 decode_spherical_16(uint16_t data){
    uint32_t d = data;
    glm::f32vec2 v = glm::f32vec2(d & 255u, d >> 8) / 255.0f;
    v.x = 2.0 * v.x - 1.0;
    v *= 3.141593;
    return normalize(glm::f32vec3(sin(v.y) * cos(v.x), cos(v.y), sin(v.y) * sin(v.x)));
}
