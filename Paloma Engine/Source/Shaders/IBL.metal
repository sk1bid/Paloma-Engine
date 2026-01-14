//
//  IBL.metal
//  Paloma Engine
//
//  Created by Artem on 12.01.2026.
//

#include <metal_stdlib>
#include "ShaderTypes.h"
#include "Common.metal"

using namespace metal;

float3 getCubeDirection(uint face, float2 uv) {
    // UV (0,0 → 1,1) конвертируем в (-1,-1 → 1,1)
    float2 ndc = uv * 2.0 - 1.0;
    
    float3 dir;
    switch (face) {
        case 0: dir = float3( 1.0, -ndc.y, -ndc.x); break; // +X
        case 1: dir = float3(-1.0, -ndc.y,  ndc.x); break; // -X
        case 2: dir = float3( ndc.x,  1.0,  ndc.y); break; // +Y
        case 3: dir = float3( ndc.x, -1.0, -ndc.y); break; // -Y
        case 4: dir = float3( ndc.x, -ndc.y,  1.0); break; // +Z
        case 5: dir = float3(-ndc.x, -ndc.y, -1.0); break; // -Z
    }
    return normalize(dir);
}

kernel void CubeFromEquirectangular(
    texture2d<float, access::sample> equirectTexture [[texture(0)]],
    texturecube<float, access::write> cubeTexture [[texture(1)]],
    uint3 gid [[thread_position_in_grid]]
) {
    
    uint face = gid.z;
    uint size = cubeTexture.get_width();
    
    if (gid.x >= size || gid.y >= size || face >= 6) return;
    
    float2 uv = (float2(gid.xy) + 0.5) / float(size);
    
    float3 direction = getCubeDirection(face, uv);
    
    // Sample equirectangular
    constexpr sampler s(filter::linear);
    float3 d = normalize(direction);
    float2 equirectUV = float2(
        (atan2(d.z, d.x) + M_PI_F) / (2.0 * M_PI_F),
        acos(d.y) / M_PI_F
    );
    
    float4 color = equirectTexture.sample(s, equirectUV);
    
    cubeTexture.write(color, gid.xy, face);
}

// Hammersley sequence for quasi-random sampling
float2 Hammersley(uint i, uint N) {
    uint bits = i;
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    float rdi = float(bits) * 2.3283064365386963e-10;
    return float2(float(i) / float(N), rdi);
}
// GGX Importance Sampling
float3 ImportanceSampleGGX(float2 Xi, float3 N, float roughness) {
    float a = max(roughness * roughness, 0.001);
    
    float phi = 2.0 * M_PI_F * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    
    // Tangent space vector
    float3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;
    
    // Tangent space -> World space
    float3 up = abs(N.z) < 0.999 ? float3(0, 0, 1) : float3(1, 0, 0);
    float3 tangent = normalize(cross(up, N));
    float3 bitangent = cross(N, tangent);
    
    return normalize(tangent * H.x + bitangent * H.y + N * H.z);
}

kernel void PrefilterEnvironmentMap(
    texturecube<float, access::sample> sourceTexture [[texture(0)]],
    texturecube<float, access::write> destTexture [[texture(1)]],
    constant float& roughness [[buffer(0)]],
    uint3 gid [[thread_position_in_grid]]
) {
    uint face = gid.z;
    uint size = destTexture.get_width();
    
    if (gid.x >= size || gid.y >= size || face >= 6) return;
    
    float2 uv = (float2(gid.xy) + 0.5) / float(size);
    float3 N = getCubeDirection(face, uv);
    float3 R = N;
    
    constexpr sampler cubeSampler(filter::linear, mip_filter::linear);
    const uint SAMPLE_COUNT = 1024u;
    
    float3 prefilteredColor = float3(0.0);
    float totalWeight = 0.0;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(N, H) * H - N);
        
        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefilteredColor += sourceTexture.sample(cubeSampler, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    
    prefilteredColor /= max(totalWeight, 0.001);
    destTexture.write(float4(prefilteredColor, 1.0), gid.xy, face);
}
