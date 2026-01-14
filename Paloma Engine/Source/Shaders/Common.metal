//
//  ToneMapping.metal
//  Paloma Engine
//
//  Created by Artem on 14.01.2026.
//

#ifndef ToneMapping_h
#define ToneMapping_h
#include <metal_stdlib>
using namespace metal;
inline float3 ToneMapACES(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// Equirectangular sampling
inline float4 equirectangularSample(float3 direction, texture2d<float> tex) {
    constexpr sampler s(filter::linear);
    float3 d = normalize(direction);
    float2 uv = float2(
                       (atan2(d.z, d.x) + M_PI_F) / (2.0 * M_PI_F),
                       acos(d.y) / M_PI_F
                       );
    return tex.sample(s, uv);
}
#endif
