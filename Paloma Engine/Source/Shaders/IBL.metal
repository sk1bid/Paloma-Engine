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

float GeometrySchlickGGX_IBL(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) / 2.0;
    
    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / denom;
}
float GeometrySmith_IBL(float NdotV, float NdotL, float roughness) {
    float ggx2 = GeometrySchlickGGX_IBL(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX_IBL(NdotL, roughness);
    return ggx1 * ggx2;
}
kernel void IntegrateBRDF(
    texture2d<float, access::write> lut [[texture(0)]],
    uint2 gid [[thread_position_in_grid]]
) {
    uint size = lut.get_width();
    if (gid.x >= size || gid.y >= size) return;
    
    float NdotV = (float(gid.x) + 0.5) / float(size);
    float roughness = (float(gid.y) + 0.5) / float(size);
    
    // Clamp NdotV to avoid divide by zero
    NdotV = max(NdotV, 0.001);
    
    float3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;
    
    float3 N = float3(0.0, 0.0, 1.0);
    
    float A = 0.0;  // scale
    float B = 0.0;  // bias
    
    const uint SAMPLE_COUNT = 1024u;
    
    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        float2 Xi = Hammersley(i, SAMPLE_COUNT);
        float3 H = ImportanceSampleGGX(Xi, N, roughness);
        float3 L = normalize(2.0 * dot(V, H) * H - V);
        
        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);
        
        if (NdotL > 0.0) {
            float G = GeometrySmith_IBL(NdotV, NdotL, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);
            
            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    
    lut.write(float4(A, B, 0.0, 1.0), gid);
}



// Pre-scaled coefficients = Y_lm × A_l
constant float P0 = 0.886227f;   // 0.282095 × π
constant float P1 = 1.023326f;   // 0.488603 × 2π/3
constant float P2 = 0.858086f;   // 1.092548 × π/4
constant float P3 = 0.247708f;   // 0.315392 × π/4
constant float P4 = 0.429043f;   // 0.546274 × π/4


// Accurate solid angle for cubemap texel
float texelSolidAngle(float u, float v) {
    // u, v in range [-1, 1]
    float x = u;
    float y = v;
    float denom = (x * x + y * y + 1.0f);
    return 4.0f / (denom * sqrt(denom));
}

// Threadgroup size must match dispatch
#define SH_THREADGROUP_SIZE 64

kernel void ProjectToSH(
    texturecube<float, access::read> envMap [[texture(0)]],
    device atomic_float* shCoeffs [[buffer(0)]],
    constant uint& cubemapSize [[buffer(1)]],
    uint3 gid [[thread_position_in_grid]],
    uint lid [[thread_index_in_threadgroup]],
    uint3 tgid [[threadgroup_position_in_grid]]
) {
    // Threadgroup shared memory for local reduction
    threadgroup float3 localSH[9][SH_THREADGROUP_SIZE];
    
    // Initialize local accumulators
    for (int i = 0; i < 9; i++) {
        localSH[i][lid] = float3(0.0f);
    }
    
    uint size = cubemapSize;
    uint face = gid.z;
    
    if (gid.x < size && gid.y < size && face < 6) {
        float2 uv = (float2(gid.xy) + 0.5) / float(size);

        float2 ndc = uv * 2.0 - 1.0;

        float3 dir = getCubeDirection(face, uv);

        float solidAngle = texelSolidAngle(ndc.x, ndc.y) / float(size * size);
        float3 color = envMap.read(gid.xy, face).rgb;

        float3 weighted = color * solidAngle;

        localSH[0][lid] = weighted * P0;
        localSH[1][lid] = weighted * P1 * dir.y;
        localSH[2][lid] = weighted * P1 * dir.z;
        localSH[3][lid] = weighted * P1 * dir.x;
        localSH[4][lid] = weighted * P2 * dir.x * dir.y;
        localSH[5][lid] = weighted * P2 * dir.y * dir.z;
        localSH[6][lid] = weighted * P3 * (3*dir.z*dir.z - 1);
        localSH[7][lid] = weighted * P2 * dir.x * dir.z;
        localSH[8][lid] = weighted * P4 * (dir.x*dir.x - dir.y*dir.y);
    }
    
    threadgroup_barrier(mem_flags::mem_threadgroup);
    for (uint stride = 64/2; stride > 0; stride /= 2) {
        if (lid < stride) {
            for (int i = 0; i < 9; i++) {
                localSH[i][lid] += localSH[i][lid + stride];
            }
        }
        threadgroup_barrier(mem_flags::mem_threadgroup);
    }
    
    if (lid == 0) {
        for (int i = 0; i < 9; i++) {
            atomic_fetch_add_explicit(shCoeffs + i * 3 + 0, localSH[i][0].x, memory_order_relaxed);
            atomic_fetch_add_explicit(shCoeffs + i * 3 + 1, localSH[i][0].y, memory_order_relaxed);
            atomic_fetch_add_explicit(shCoeffs + i * 3 + 2, localSH[i][0].z, memory_order_relaxed);
        }
    }
}
