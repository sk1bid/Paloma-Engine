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
                            constant FrameUniforms& uniforms [[buffer(BufferIndexUniforms)]],
                            constant InstanceData& instanceData [[buffer(BufferIndexInstanceData)]]
                            )
{
    VertexOut out;
    
    // Transform vertex position using model matrix
    /// Transfer object from local space (0,0,0) to World position
    float4 worldPos = instanceData.modelMatrix * float4(in.position, 1.0);
    
    // Normal direction (perpendicular to surface)
    /// Normals shoud rotate with object for correct lighting
    float3 worldNormal = (instanceData.normalMatrix * float4(in.normal, 0.0)).xyz;
    
    // Transform to clip space: Projection * View * World
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    
    out.worldPos = worldPos.xyz;
    out.normal = normalize(worldNormal);
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
