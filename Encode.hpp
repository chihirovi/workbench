#pragma once

#include <stdio.h>
#include <glm/glm.hpp>
#include <random>
#include <tuple>

struct PackedNormal{
    uint32_t x = 0.0f;
    uint32_t y = 0.0f;
    float sign;
};

struct PackedTangent{
    uint32_t angle;
    float sign;
};

PackedNormal SignedOctEncode(glm::vec3 normal){
    glm::vec2 xy_in_oct_surfect 
        = glm::vec2(normal.x, normal.y) / (abs(normal.x) + abs(normal.y) + abs(normal.z));
    
    return PackedNormal{
        .x = uint32_t((xy_in_oct_surfect.x - xy_in_oct_surfect.y + 1.0f) * 0.5f * 1024.0f),
        .y = uint32_t((xy_in_oct_surfect.x + xy_in_oct_surfect.y + 1.0f) * 0.5f * 1024.0f),
        .sign = normal.z > 0.0f ? 1.0f : -1.0f,
    };
}

glm::vec3 SignedOctDecode(PackedNormal pn){
    float fx = float(pn.x)/1024.f;
    float fy = float(pn.y)/1024.f;
    float x = (fx + fy - 1.0f);
    float y = (fy - fx);
    float z = pn.sign * (1.0f - abs(x) - abs(y));
    
    return glm::normalize(glm::vec3(x, y, z));
}

PackedTangent SignedDiamondEncode(glm::vec2 v){
    float x_in_diamond = v.x / (abs(v.x) + abs(v.y));
    
    return PackedTangent{
        .angle = uint32_t(((x_in_diamond + 1.0f) / 2.0f) * 1024.0f),
        .sign = v.y > 0.0f ? 1.0f : -1.0f,
    };
}

glm::vec2 SignedDiamondDecode(PackedTangent t){
    float x = float(t.angle)/1024.f * 2.0f - 1.0f;
    float y = t.sign * (1.0f - abs(x));
    return glm::normalize(glm::vec2(x, y));
}

std::pair<PackedNormal, PackedTangent> EncodeNT(glm::vec3 n, glm::vec3 t){
    glm::vec3 tb1;
    if(abs(n.x) > abs(n.y)){
        tb1 = {-n.y, n.x, 0.0f};
    }else{
        tb1 = {0.0f, -n.z, n.y};
    }
    tb1 = glm::normalize(tb1);
    auto tb2 = glm::cross(tb1, n);

    glm::vec2 pt = {
        glm::dot(t, tb1),
        glm::dot(t, tb2),
    };

    return {
        SignedOctEncode(n),
        SignedDiamondEncode(pt), 
    };
}

std::tuple<glm::vec3, glm::vec3> DecodeNT(std::pair<PackedNormal, PackedTangent> PNT){

    glm::vec3 n = SignedOctDecode(PNT.first);
    glm::vec2 pt = SignedDiamondDecode(PNT.second);

    glm::vec3 tb1;
    if(abs(n.x) > abs(n.y)){
        tb1 = {-n.y, n.x, 0.0f};
    }else{
        tb1 = {0.0f, -n.z, n.y};
    }
    tb1 = glm::normalize(tb1);
    auto tb2 = glm::cross(tb1, n);

    auto t = pt.x * tb1 + pt.y * tb2;

    return {n, t};
}

glm::vec3 Orthogonalize(glm::vec3 v1, glm::vec3 v2){
    return glm::normalize(v2 - glm::dot(v2, v1) * v1);
}

void TestOct(void){

    for(int i=0; i<16; i++){

        glm::vec3 n = {
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
        };
        n = glm::normalize(n);

        glm::vec3 t = {
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
            (rand() / (RAND_MAX + 1.0)) * 2.0f - 1.0f,
        };
        t = glm::normalize(t);
        t = Orthogonalize(n, t);
        
        printf("%f %f %f\n", n.x, n.y, n.z);
        printf("%f %f %f\n", t.x, t.y, t.z);
        
        auto [nn, tt] = DecodeNT(EncodeNT(n, t));
        printf("%f %f %f\n", nn.x, nn.y, nn.z);
        printf("%f %f %f\n", tt.x, tt.y, tt.z);
        printf("\n");
    }
}