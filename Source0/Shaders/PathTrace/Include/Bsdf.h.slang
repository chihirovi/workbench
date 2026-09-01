#pragma once
#include "Lambert.h.slang"
#include "GGX.h.slang"
#include "../../Include/Constant.h.slang"

struct BSDF {
    float roughness;
    float metalness;
    float3 normal;
    float3 albedo;
    float3 omega_o;

    __init(float r, float m, float3 n, float3 a, float3 v) {
        roughness = r;
        metalness = m;
        normal = n;
        albedo = a;
        omega_o = v;
    }
    
    float3 eval_fs(float3 dir){

        float3 specular_brdf;
        
        if(roughness == 0.0f){ //roughness = 0のとき，deltaが出現する，fd + fs のfdがdeltaで消される
            // roughness = 0で，metal成分があって，角度が反射成分に一致したら，delta->infになる
            if(metalness != 0.0 && all(dir == reflect(-omega_o, normal))) return albedo * metalness;
            else specular_brdf = float3(0.0);

        } else {
            specular_brdf = ggx_smith(albedo, metalness, roughness, normal, dir, omega_o);
        }

        return albedo/M_PI * (1.0f - metalness) + specular_brdf*metalness;
        //return albedo/M_PI;
    }

    float eval_pdf(float3 dir) {
        //float pdf = max(dot(dir, normal)/M_PI, 1e-5);
        //return pdf;
        if(roughness == 0.0f && metalness == 1.0f) return 1.0f;

        float pdf_specular;
        if (roughness == 0.0f) {
            // roughness = 0で，metal成分があって，角度が反射成分に一致したら，delta->infになる
            if(metalness != 0.0 && all(dir == reflect(-omega_o, normal))) return metalness;
            else pdf_specular = 0.0;

        }else{
            pdf_specular = ggxPdf(normal, omega_o, dir, roughness);
        }

        float pdf_diffuse = max(dot(dir, normal)/M_PI, 1e-10);

        return pdf_diffuse * (1.0f - metalness) + pdf_specular * metalness;
    }
    
    void sample_fs(float4 rand, out float3 out_dir){
            
        //sample ω_i
        //float3 out_omega_i = sampleCosineHemisphere(normal, rand.xy);
        //dir = out_omega_i;
        //return;
        
        //update alpha(= fs * cos / pdf)
        // cosinePDF = cos/pi;
        //alpha = albedo/pi * cos / cosinePDF; -> albedo
        
        
        
        bool is_diffuse = rand[0] <= (1.0-metalness) ? true : false;

        if(is_diffuse){
            float3 out_omega_i = sampleCosineHemisphere(normal, float2(rand[1], rand[2]));
            out_dir = out_omega_i;

        } else {

            if(roughness == 0.0f){
                out_dir = reflect(-omega_o, normal);
                return;
            }    
            float3 out_omega_i = sampleGGX(normal, omega_o, roughness, float2(rand[1], rand[2]));
            out_dir = out_omega_i;
        }
    }
};