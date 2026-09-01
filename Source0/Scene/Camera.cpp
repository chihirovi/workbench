#include "Camera.hpp"

namespace Irori::Scene{
    
Camera::Camera(
    uint32_t _w, 
    uint32_t _h, 
    glm::f32vec3 _lookfrom, 
    glm::f32vec3 _lookat, 
    float _fov, 
    glm::f32vec3 _vup){
    
    image_w = _w, image_h = _h;
    
    lookfrom = lookfrom_init = _lookfrom;
    lookat_d = lookat_d_init = glm::normalize(_lookat - _lookfrom);
    vup = vup_init = _vup;
    fov = _fov;
    
    update_projection_matrix();
    update_view_matrix();
}

void Camera::update(float _time_delta, bool* key_response, float *mouse_offset){
    pre_lookfrom = lookfrom;
    pre_lookat_d = lookat_d;
    pre_vup = vup;

    time_delta = _time_delta*10;

    int dx = 0, dy = 0, dz = 0;
    if (key_response[0]) { dz += 1; } //w front
    if (key_response[1]) { dx -= 1; } //a left
    if (key_response[2]) { dz -= 1; } //s back
    if (key_response[3]) { dx += 1; } //d right
    if (key_response[4]) { dy += 1; } //space up 
    if (key_response[5]) { dy -= 1; } //x dawn

    
    update_origin(dx, dy, dz);

    update_lookat_dir(mouse_offset);

    if (key_response[6]) { reset(); }
    
    update_view_matrix();
    update_projection_matrix();
}

//x y z must == (0 or -1 or 1)
void Camera::update_origin(int dx, int dy, int dz) {

    //left right
    if (dx != 0) {
        float fdx = dx*origin_sensitivity[0];
        auto right = -cross(vup,lookat_d);
        glm::normalize(right);
        lookfrom += right*fdx*time_delta;
    }

    //up down
    if (dy != 0) {
        float fdy = dy*origin_sensitivity[1];
        lookfrom += vup*fdy*time_delta;
    }

    //flont back
    if (dz != 0) {
        float fdz = dz*origin_sensitivity[2];
        auto d = glm::normalize(lookat_d);
        lookfrom += d*fdz*time_delta;
    }	
}

//mdx, mdy : pixel
void Camera::update_lookat_dir(float* mo) {

    float m_dx = mo[0], m_dy = mo[1];

    if (m_dx != 0) {
        float fdx = m_dx*lookat_sensitivity[0];

        auto left = glm::cross(lookat_d,vup);
        glm::normalize(left);
        lookat_d += left*fdx*time_delta;
        glm::normalize(lookat_d);
    }

    if (m_dy != 0) {
        float fdy   = m_dy*lookat_sensitivity[1];
        auto left   = glm::cross(lookat_d,vup);
        auto tup    = glm::cross(lookat_d,left);
        lookat_d    += tup*fdy*time_delta;
        glm::normalize(lookat_d);
    }
    
    //limit camera y angle
    if (lookat_d[1] > sin80 ) {
        lookat_d[1] = sin80;
    }

    if (-sin80 > lookat_d[1]) {
        lookat_d[1] = -sin80;
    }
}	

void Camera::update_projection_matrix(){
    projection = glm::perspective(
        glm::radians(fov),
        (float)image_w/(float)image_h,
        z_near,
        z_far
    );
    //window origin openGL left up: +y , zclip: -1<1
    //window origin vulkan left bottom: -y , zclip:  0<1
    //vulkanでは画面への書き込み(ビューポートあたり)でYが反転するみたい
    //これは座標系的な理由というより，ただのビューポートの問題？
    //あと三角形のcullingが逆になる
    projection[1][1] *= -1; //Y axis reverse
}

void Camera::update_view_matrix(){
    view = glm::lookAt(
        lookfrom,
        lookfrom+lookat_d,
        vup
    );
}

void Camera::reset(){
    lookfrom = lookfrom_init;
    lookat_d = lookat_d_init;
    vup      = vup_init;
}

}