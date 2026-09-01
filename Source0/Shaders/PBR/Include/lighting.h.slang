static float M_PI = 3.141592653589793;

float3 schlick_fresnel(float3 specular, float VH) {
    return specular + (1.0 - specular) * pow((1.0f - VH), 5.0f);
}

float D_beckmann(float m, float NH) {
    float c2 = NH * NH;
    float c4 = c2 * c2;
    float m2 = m * m;
    return (1.0f / (m2 * c4) * exp((-1.0f / m2) * (1.0f - c2) / c2));
}

float G2_vcavity(float NH, float NV, float NL, float VH) {
    return min(1.0f, min(2.0f * NH * NV / VH,  2.0f * NH * NL / VH));
}

float D_GGX(float m2, float NH) {
    float f = (NH * m2 - NH) * NH + 1;
    return m2/(M_PI * f * f);
}

float G2_smith(float NL, float NV, float m2) {
    float NL2 = NL * NL;
    float NV2 = NV * NV;
    float lambda_v = (-1.0f + sqrt(m2 * (1.0f - NL2) / NL2 + 1.0f)) * 0.5f;
    float lambda_l = (-1.0f + sqrt(m2 * (1.0f - NV2) / NV2 + 1.0f)) * 0.5f;
    return 1.0f / (1.0f + lambda_v + lambda_l);
}

float3 cook_trance(float3 base_color, float metalness, float roughness, float3 N, float3 L, float3 V, float light_visibility) {
    if(light_visibility == 0.0) return float3(0.0);

    float3 H = normalize(V+L);
    
    float NH = saturate(dot(N, H));
    float NV = saturate(dot(N, V));
    float NL = saturate(dot(N, L));
    float VH = saturate(dot(V, H));
    
    float3 Ks = base_color*metalness;
    float a = roughness * roughness;
    float D = D_beckmann(a, NH);
    float G2 = G2_vcavity(NH, NV, NL, VH);
    float3 Fr = schlick_fresnel(lerp(0.04, base_color, metalness), VH);
    
    float3 specular = (D * G2 * Fr)/(4.0f * NV * NL);
    return saturate(specular);
}

// specular brdf
float3 ggx_smith(float3 base_color, float metalness, float roughness, float3 N, float3 L, float3 V, float light_visibility) {
    if(light_visibility == 0.0) return float3(0.00);

    float3 H = normalize(V+L);
    
    float NH = saturate(dot(N, H));
    float NV = saturate(dot(N, V));
    float NL = saturate(dot(N, L));
    float VH = saturate(dot(V, H));
    
    float3 Ks = base_color*metalness;
    float a = roughness * roughness;
    float m2 = a * a;
    float D = D_GGX(m2, NH);
    float G2 = G2_smith(NL, NV, m2);
    float3 Fr = schlick_fresnel(lerp(0.04, base_color, metalness), VH);
    //float3 Fr = schlick_fresnel(Ks, NL);
    
    float3 specular = (D * G2 * Fr)/(4.0f * NV * NL);
    //return saturate(specular);
    return specular;
}

float3 phone(float3 base_color, float shininess, float metalness, float3 N, float3 L, float3 V) {
    float3 R = normalize(-V+2*dot(N,V)*N);
    float NL = saturate(dot(N, L));
    float LR = saturate(dot(R, L));
    
    float3 specular = base_color * metalness*((shininess+2.0)/(2.0 * M_PI))*pow(LR, shininess);
    return specular;
}


//-------------------
//-------------------
// D（GGX）の項
float D_GGX_2(float3 H, float3 N, float roughness_) {
    float NdotH = saturate(dot(H, N));
    float roughness = saturate(roughness_);
    float alpha = roughness * roughness;
    float alpha2 = alpha * alpha;
    float t = ((NdotH * NdotH) * (alpha2 - 1.0) + 1.0);
    float PI = 3.1415926535897;
    return alpha2 / (PI * t * t);
}

// フレネルの項
float3 Flesnel(float3 V, float3 H, float3 F0) {
    float VdotH = saturate(dot(V, H));
    float3 F = pow(1.0 - VdotH, 5.0);
    F *= (1.0 - F0);
    F += F0;
    return F;
}

// G - 幾何減衰の項（クック トランスモデル）
float G_CookTorrance(float3 L, float3 V, float3 H, float3 N) {
    float NdotH = saturate(dot(N, H));
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float VdotH = saturate(dot(V, H));

    float NH2 = 2.0 * NdotH;
    float g1 = (NH2 * NdotV) / VdotH;
    float g2 = (NH2 * NdotL) / VdotH;
    float G = min(1.0, min(g1, g2));
    return G;
}

float3 cook_torrance_2(float3 base_color, float metalness, float roughness, float3 N, float3 L, float3 V) {
    float3 halfVector = normalize(L + V);

    // D_GGXの項
    float D = D_GGX_2(halfVector, N, roughness);

    // Fの項
    float3 F = Flesnel(V, halfVector, lerp(float3(0.04), base_color, metalness));

    // Gの項
    float G = G_CookTorrance(L, V, halfVector, N);

    // スペキュラおよびディフューズを計算
    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));

    float3 specularReflection = (D * F * G) / (4.0 * NdotV * NdotL + 0.000001);
    return specularReflection;
}


