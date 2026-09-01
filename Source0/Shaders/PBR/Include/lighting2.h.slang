// defines
#define PI 3.14159265359
#define PI2 6.28318530718
#define RECIPROCAL_PI 0.31830988618
#define RECIPROCAL_PI2 0.15915494
#define LOG2 1.442695
#define EPSILON 1e-6

// lights

bool testLightInRange(const in float lightDistance, const in float cutoffDistance) {
  return any(bool2(cutoffDistance == 0.0, lightDistance < cutoffDistance));
}

float punctualLightIntensityToIrradianceFactor(const in float lightDistance, const in float cutoffDistance, const in float decayExponent) {
  if (decayExponent > 0.0) {
    return pow(saturate(-lightDistance / cutoffDistance + 1.0), decayExponent);
  }
  
  return 1.0;
}

struct PointLight {
  float3 position;
  float3 color;
  float distance;
  float decay; //5?
};

float3 getPointDirectLightIrradiance(PointLight pointLight, float3 L) {
    float3 color = pointLight.color;
    color *= punctualLightIntensityToIrradianceFactor( pointLight.distance, 100, pointLight.decay);
    return color;
    /*
  if (testLightInRange(pointLight.distance, pointLight.distance)) {
    float3 color = pointLight.color;
    color *= punctualLightIntensityToIrradianceFactor( pointLight.distance, pointLight.distance, pointLight.decay);
    return color;
  } else {
    return float3(0.0);
  }
  */
}
// BRDFs

float3 F_Schlick(float3 specularColor, float3 H, float3 V) {
  return (specularColor + (1.0 - specularColor) * pow(1.0 - saturate(dot(V,H)), 5.0));
}

float D_GGX_(float a, float dotNH) {
  float a2 = a*a;
  float dotNH2 = dotNH*dotNH;
  float d = dotNH2 * (a2 - 1.0) + 1.0;
  return a2 / (PI * d * d);
}

float G_Smith_Schlick_GGX(float a, float dotNV, float dotNL) {
  float k = a*a*0.5 + EPSILON;
  float gl = dotNL / (dotNL * (1.0 - k) + k);
  float gv = dotNV / (dotNV * (1.0 - k) + k);
  return gl*gv;
}

// Cook-Torrance
float3 SpecularBRDF(float3 N, float3 V, float3 L, float3 specularColor, float roughnessFactor) {
  
  float dotNL = saturate(dot(N,L));
  float dotNV = saturate(dot(N,V));
  float3 H = normalize(L+V);
  float dotNH = saturate(dot(N,H));
  float dotVH = saturate(dot(V,H));
  float dotLV = saturate(dot(L,V));
  float a = roughnessFactor * roughnessFactor;

  float D = D_GGX_(a, dotNH);
  float G = G_Smith_Schlick_GGX(a, dotNV, dotNL);
  float3 F = F_Schlick(specularColor, V, H);
  return (F*(G*D))/(4.0*dotNL*dotNV+EPSILON);
}