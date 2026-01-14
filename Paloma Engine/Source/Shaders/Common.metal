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
#endif
