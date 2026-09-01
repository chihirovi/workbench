#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>


uint16_t encode_spherical_16(glm::f32vec3 nor);
glm::f32vec3 decode_spherical_16(uint16_t data);
