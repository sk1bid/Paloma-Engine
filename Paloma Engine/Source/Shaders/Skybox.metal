//
//  Skybox.metal
//  Paloma Engine
//
//  Created by Artem on 14.01.2026.
//

#include <metal_stdlib>
#include "ShaderTypes.h"
#include "Common.metal"

using namespace metal;

struct SkyboxOut {
    float4 position [[position]]; // clip space position
    float3 direction;
};


vertex SkyboxOut skyboxVertex(
                              uint vid [[vertex_id]],
                              constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]]
                              ) {
    
    float2 gridPos = float2((vid << 1) & 2, vid & 2);
    float2 positions = gridPos * 2.0 - 1.0;
    
    SkyboxOut out;
    out.position = float4(positions, 1.0, 1.0);
    
    float4 clipPos = float4(positions, 1.0, 1.0);
    
    float4 viewPos = uniforms.projectionMatrixInverse * clipPos;
    viewPos /= viewPos.w;
    viewPos.w = 0.0;
    
    out.direction = (uniforms.viewMatrixInverse * viewPos).xyz;
    
    return out;
}

fragment float4 skyboxFragment(
    SkyboxOut in [[stage_in]],
    constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]]
) {
    device const EnvironmentData& env = *(device const EnvironmentData*)uniforms.environmentAddress;
    
    constexpr sampler cubeSampler(filter::linear, mip_filter::linear);

    float3 color = env.skyboxMap.sample(cubeSampler, in.direction).rgb;
    
    // ACES + gamma
    color = ToneMapACES(color);
    color = pow(color, float3(1.0/2.2));
    
    return float4(color, 1.0);
}
