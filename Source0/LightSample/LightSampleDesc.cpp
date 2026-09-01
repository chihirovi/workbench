#include "LightSampleDesc.hpp"

namespace Irori::WA_LightSample{

void LightSampleDesc::set_data(
    uint32_t    light_object_num, 
    uint64_t    light_object_pdf_wa_table_addr,     //WA_Entry*
    uint64_t    light_object_table_addr,             //LightObject* 
    uint64_t    light_primitive_pdf_wa_table_addr    //WA_Entry*   
){
    gpu_desc.light_object_num = light_object_num;
    gpu_desc.padding0 = 0,
    gpu_desc.light_object_pdf_wa_table = light_object_pdf_wa_table_addr;
    gpu_desc.light_object_table = light_object_table_addr;
    gpu_desc.light_primitive_pdf_wa_table = light_primitive_pdf_wa_table_addr;
}

void LightSampleDesc::copy_to_buffer(Core::UniqueBuffer& buffer){
    buffer.transfer_data(&gpu_desc, sizeof(GPU_LightSample_DESC));
}

}