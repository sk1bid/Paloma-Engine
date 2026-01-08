//
//  Render.metal
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include <metal_stdlib>
#include <simd/simd.h>
using namespace metal;


#include "ShaderTypes.h"

// -- Transfer Data Structures --

struct VertexIn {
    float3 position [[attribute(VertexAttributePosition)]];
    float3 normal [[attribute(VertexAttributeNormal)]];
    float2 texcoord [[attribute(VertexAttributeTexcoord)]];
    
};

struct VertexOut {
    float4 position [[position]]; // clip space position
    float3 worldPos; // for lightning
    float3 normal;
    float2 texcoord; // UV for textures
};


// -- Vertex Shader --
vertex VertexOut vertexMain(
    VertexIn in [[stage_in]],
    constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]]
) {
    VertexOut out;
    
    float angleY = uniforms.time;
    float angleX = uniforms.time * 0.5;
    
    // Rotation around Y axis
    float cosY = cos(angleY);
    float sinY = sin(angleY);
    
    float3 pos1;
    pos1.x = in.position.x * cosY + in.position.z * sinY;
    pos1.y = in.position.y;
    pos1.z = -in.position.x * sinY + in.position.z * cosY;
    
    // Rotation around X axis
    float cosX = cos(angleX);
    float sinX = sin(angleX);
    
    float3 rotatedPos;
    rotatedPos.x = pos1.x;
    rotatedPos.y = pos1.y * cosX - pos1.z * sinX;
    rotatedPos.z = pos1.y * sinX + pos1.z * cosX;
    
    float3 norm1;
    norm1.x = in.normal.x * cosY + in.normal.z * sinY;
    norm1.y = in.normal.y;
    norm1.z = -in.normal.x * sinY + in.normal.z * cosY;
    
    float3 rotatedNormal;
    rotatedNormal.x = norm1.x;
    rotatedNormal.y = norm1.y * cosX - norm1.z * sinX;
    rotatedNormal.z = norm1.y * sinX + norm1.z * cosX;
    
    float4 worldPos = float4(rotatedPos, 1.0);
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    
    out.worldPos = worldPos.xyz;
    out.normal = rotatedNormal;
    out.texcoord = in.texcoord;
    
    return out;
}

// -- Fragment Shader (debug) --
fragment float4 fragmentMain (
                              VertexOut in [[stage_in]]
                              )
{
    float3 color = in.normal * 0.5 + 0.5;
    return float4(color, 1.0);
}
