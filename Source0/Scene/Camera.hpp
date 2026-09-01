#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Irori::Scene{

class Camera{
public:
    //camera move bias
    float sin80 = sin(80.0/360.0 * 2 * 3.14159265); //up_limit
	float origin_sensitivity[3] = { 0.12,0.12,0.12 };
	float lookat_sensitivity[2] = { 0.001,0.001};
    float time_delta = 0;
    
    //init data
    glm::f32vec3 lookfrom_init;      
    glm::f32vec3 lookat_d_init;        
    glm::f32vec3 vup_init;           

    //const data
    uint32_t image_w, image_h;
    float fov;      //degree
    float z_near = 0.1;
    float z_far = 100;
    
    //update par frame
    glm::f32vec3 lookfrom;      
    glm::f32vec3 lookat_d;        
    glm::f32vec3 vup;           

    glm::f32vec3 pre_lookfrom   = {0.0f, 0.0f, 0.0f};      
    glm::f32vec3 pre_lookat_d   = {0.0f, 0.0f, 1.0f};        
    glm::f32vec3 pre_vup        = {0.0f, 1.0f, 0.0f};           
    
    //matrix
    glm::f32mat4x4 view;
    glm::f32mat4x4 projection; 

public:
    Camera(){;}
    Camera(uint32_t _w, uint32_t _h, glm::f32vec3 _lookfrom, glm::f32vec3 _lookat, float _fov, glm::f32vec3 _vup);
    void update(float _time_delta, bool* key_response, float* mouse_offset);
    void update_origin(int dx, int dy, int dz);
    void update_lookat_dir(float* mo);
    void update_projection_matrix();
    void update_view_matrix();
    void reset();
};
    
}
