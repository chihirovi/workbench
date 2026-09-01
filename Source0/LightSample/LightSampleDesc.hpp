#pragma once
#include "../Core/Common/Buffer.hpp"
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/quaternion.hpp>
#include <random>

#define DDGI_IRRADIANCE_NUM_TEXELS_INCLUDE_BORDER 8
#define DDGI_DISTANCE_NUM_TEXELS_INCLUDE_BORDER 16
#define DDGI_PROBE_NUM_RAY 192

namespace Irori::WA_LightSample{

struct GPU_LightSample_DESC{
    uint32_t    light_object_num;
    uint32_t    padding0;
    uint64_t    light_object_pdf_wa_table; //WA_Entry* 
    uint64_t    light_object_table; //LightObject* 
    uint64_t    light_primitive_pdf_wa_table; //WA_Entry*   
};
    
class LightSampleDesc{
public:
    GPU_LightSample_DESC gpu_desc;

public:
    LightSampleDesc(){;}
    void set_data(
        uint32_t    light_object_num, 
        uint64_t    light_object_pdf_wa_table_addr,      //WA_Entry*
        uint64_t    light_object_table_addr,             //LightObject* 
        uint64_t    light_primitive_pdf_wa_table_addr    //WA_Entry*   
    );
    void copy_to_buffer(Core::UniqueBuffer& buffer);
};

};