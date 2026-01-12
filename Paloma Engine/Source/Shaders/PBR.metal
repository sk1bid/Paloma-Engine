//
//  Render.metal
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include <metal_stdlib>
#include <simd/simd.h>
#include "ShaderTypes.h"

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

float3 ToneMapACES(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
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
    // get material
    uint64_t addr = instances[in.instanceID].materialAddress;
    device const MaterialArguments &material = *(device const MaterialArguments*)addr;
    
    // vectors in world
    float3 V = normalize(uniforms.cameraPosition - in.worldPos);
    float3 L = normalize(float3(1.0, 1.0, 1.0));
    float3 H = normalize(V + L);
    
    
    float2x3 TB = cotangents(in.normal, in.worldPos, in.texcoord);
    float3 nt = material.normalTexture.sample(textureSampler, in.texcoord).rgb * 2.0 - 1.0;
    float3 N = normalize(TB[0] * nt.x + TB[1] * nt.y + in.normal * nt.z);
    
    // material settings
    float metallic = material.constants.metallicFactor;
    if (material.metalnessTexture.get_width() > 0) {
        metallic *= material.metalnessTexture.sample(textureSampler, in.texcoord).r;
    }
    float roughness = material.constants.roughnessFactor;
    if (material.roughnessTexture.get_width() > 0) {
        roughness *= material.roughnessTexture.sample(textureSampler, in.texcoord).r;
    }
    roughness = clamp(roughness, 0.05f, 1.0f);
    float3 albedo = material.baseColorTexture
        .sample(textureSampler, in.texcoord).rgb;
    
    float3 F0 = mix(float3(0.04), albedo, metallic);
    
    float NdotL = max(dot(N, L), 0.0);
    float NdotV = max(dot(N, V), 0.001);
    float NdotH = max(dot(N, H), 0.0);
    float VdotH = max(dot(V, H), 0.0);
    float D = distributionGGX(NdotH, roughness);
    float V_vis = visibilityGGX(NdotL, NdotV, roughness);
    float3 F = fresnelSchlick(F0, VdotH);
    float3 specular = D * V_vis * F;
    float3 kD = (float3(1.0) - F) * (1.0 - metallic);
    float3 diffuse = kD * albedo / M_PI_F;

    float3 finalColor = (diffuse + specular) * NdotL * 3.0;
    finalColor += albedo * 0.03;

    finalColor *= 0.9;
    float3 mapped = ToneMapACES(finalColor);
    // --- DEBUG SECTION ---
    //return float4(N * 0.5 + 0.5, 1.0);
    //return float4(float3(roughness), 1.0);
    //return float4(specular * 5.0, 1.0);
    //return float4(diffuse, 1.0);
    return float4(pow(mapped, float3(1.0/2.2)), 1.0);
    
}
