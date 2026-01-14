//
//  Render.metal
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include <metal_stdlib>
#include <simd/simd.h>
#include "ShaderTypes.h"
#include "Common.metal"

using namespace metal;

// -- Helpers --
float2x3 cotangents(float3 N, float3 p, float2 uv) {
    float3 dp1 = dfdx(p);
    float3 dp2 = dfdy(p);
    float2 duv1 = dfdx(uv);
    float2 duv2 = dfdy(uv);
    float3 dp2perp = cross(dp2, N);
    float3 dp1perp = cross(N, dp1);
    float3 T = dp2perp * duv1.x + dp1perp * duv2.x;
    float3 B = dp2perp * -duv1.y + dp1perp * -duv2.y;
    float invmax = rsqrt(max(dot(T, T), dot(B, B)));
    return float2x3(T * invmax, B * invmax);
}

// Evaluate SH irradiance at direction n
float3 evaluateSHIrradiance(float3 n, constant SHCoefficients& sh) {
    // Константы, объединяющие Ylm и Al (Lambertian convolution)
    const float C1 = 0.429043;
    const float C2 = 0.511664;
    const float C3 = 0.743125;
    const float C4 = 0.886227;
    const float C5 = 0.247708;

    // Реконструкция по формуле Рамамурти-Ханрахана
    float3 irradiance =
        C4 * sh.L00 -
        C5 * sh.L20 +
        C3 * sh.L20 * n.z * n.z +
        C1 * sh.L22 * (n.x * n.x - n.y * n.y) +
        C1 * sh.L2m2 * n.x * n.y +
        C1 * sh.L21 * n.x * n.z +
        C1 * sh.L2m1 * n.y * n.z +
        C2 * sh.L11 * n.x +
        C2 * sh.L1m1 * n.y +
        C2 * sh.L10 * n.z;

    return max(float3(0.0), irradiance);
}
// -- Texture Sampler --
constexpr sampler textureSampler(filter::linear, mip_filter::linear, address::repeat);

// -- PBR Functions --

// D - GGX
float distributionGGX(float NdotH, float alphaRoughness) {
    float a2 = alphaRoughness * alphaRoughness;
    float d = (NdotH * NdotH) * (a2 - 1.0f) + 1.0f;
    return a2/(M_PI_F * d * d);
}

// V - Smith
float visibilityGGX(float NdotL, float NdotV, float alphaRoughness) {
    float a2 = alphaRoughness * alphaRoughness;
    float GV = NdotL * sqrt(NdotV * NdotV * (1.0f - a2) + a2);
    float GL = NdotV * sqrt(NdotL * NdotL * (1.0f - a2) + a2);
    return 0.5f / max(GV + GL, 0.001f);
}

// F - Fresnel Schlick
float3 fresnelSchlick(float3 f0, float VdotH) {
    return f0 + (1.0f - f0) * pow(clamp(1.0f - VdotH, 0.0f, 1.0f), 5.0f);
}

// -- Transfer Data Structures --

struct VertexIn {
    packed_float3 position;
    packed_float3 normal;
    float2 texcoord;
    
};

struct VertexOut {
    float4 position [[position]]; // clip space position
    float3 worldPos; // for lightning
    float3 normal;
    float2 texcoord; // UV for textures
    uint instanceID [[flat]];
};


// -- Vertex Shader --
vertex VertexOut vertexMain(
                            device const VertexIn* vertices [[buffer(BufferIndexVertices)]],
                            constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]],
                            constant InstanceData* instances[[buffer(BufferIndexInstanceData)]],
                            uint vid [[vertex_id]],
                            uint instanceID [[instance_id]]
                            )
{
    VertexOut out;
    VertexIn in_packed = vertices[vid];
    
    float3 position = float3(in_packed.position);
    float3 normal = float3(in_packed.normal);
    
    InstanceData inst = instances[instanceID];
    // Transform vertex position using model matrix
    /// Transfer object from local space (0,0,0) to World position
    float4 worldPos = inst.modelMatrix * float4(position, 1.0);
    
    // Normal direction (perpendicular to surface)
    /// Normals shoud rotate with object for correct lighting
    float3 worldNormal = (inst.normalMatrix * float4(normal, 0.0)).xyz;
    
    
    // Transform to clip space: Projection * View * World
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    
    out.worldPos = worldPos.xyz;
    out.normal = normalize(worldNormal);
    out.texcoord = in_packed.texcoord;
    out.instanceID = instanceID;
    
    return out;
    
}

// -- Fragment Shader --

fragment float4 fragmentMain (
    VertexOut in [[stage_in]],
    constant InstanceData* instances [[buffer(BufferIndexInstanceData)]],
    constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]]
) {
    
    uint64_t addr = instances[in.instanceID].materialAddress;
    device const MaterialArguments &material = *(device const MaterialArguments*)addr;
    device const EnvironmentData& env = *(device const EnvironmentData*)uniforms.environmentAddress;
    
    float3 V = normalize(uniforms.cameraPosition - in.worldPos);
    float3 L = normalize(float3(1.0, 1.0, 1.0));  // Directional light
    float3 H = normalize(V + L);
    
    // Normal mapping
    float2x3 TB = cotangents(in.normal, in.worldPos, in.texcoord);
    float3 nt = material.normalTexture.sample(textureSampler, in.texcoord).rgb * 2.0 - 1.0;
    float3 N = normalize(TB[0] * nt.x + TB[1] * nt.y + in.normal * nt.z);
    
    // Reflection vector for IBL
    float3 R = reflect(-V, N);
    
    // -- Material Properties --
    
    float metallic = material.constants.metallicFactor;
    if (material.metalnessTexture.get_width() > 0) {
        metallic *= material.metalnessTexture.sample(textureSampler, in.texcoord).r;
    }
    
    float roughness = material.constants.roughnessFactor;
    if (material.roughnessTexture.get_width() > 0) {
        roughness *= material.roughnessTexture.sample(textureSampler, in.texcoord).r;
    }
    roughness = clamp(roughness, 0.05f, 1.0f);
    
    float3 albedo = material.baseColorTexture.sample(textureSampler, in.texcoord).rgb;
    
    float3 F0 = mix(float3(0.04), albedo, metallic);
    
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    
    // Direct lighting
    
    float D = distributionGGX(NdotH, roughness);
    float V_vis = visibilityGGX(NdotL, NdotV, roughness);
    float3 F = fresnelSchlick(F0, VdotH);
    
    float3 specularDirect = D * V_vis * F;
    float3 kD = (float3(1.0) - F) * (1.0 - metallic);
    float3 diffuseDirect = kD * albedo / M_PI_F;
    
    float3 directLighting = (diffuseDirect + specularDirect) * NdotL * 3.0;
    
    // -- IBL --
    
    constexpr sampler cubeSampler(filter::linear, mip_filter::linear);
    constexpr sampler lutSampler(filter::linear);
    
    // Specular IBL
    float lod = roughness * float(env.prefilteredMap.get_num_mip_levels() - 1);
    float3 prefilteredColor = env.prefilteredMap.sample(cubeSampler, R, level(lod)).rgb;
    
    // BRDF LUT
    float2 brdf = env.brdfLut.sample(lutSampler, float2(NdotV, roughness)).rg;
    float3 specularIBL = prefilteredColor * (F0 * brdf.x + brdf.y);
    
    
    // Diffuse IBL
    float3 irradiance = evaluateSHIrradiance(N, uniforms.sh);
    float3 diffuseIBL = albedo * irradiance / M_PI_F;
    
    // Combine IBL
    float3 ambientLighting = specularIBL + diffuseIBL * (1.0 - metallic);
    
    // ========================================
    // 7. FINAL COMPOSITION
    // ========================================
    
    float3 finalColor = directLighting + ambientLighting;
    
    // Tonemapping
    float3 mapped = ToneMapACES(finalColor);
    
    // Gamma correction
    return float4(pow(mapped, float3(1.0/2.2)), 1.0);
}
