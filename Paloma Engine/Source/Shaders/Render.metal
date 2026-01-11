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

// -- Texture Sampler --
constexpr sampler textureSampler(filter::linear, mip_filter::linear, address::repeat);



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
    float3 worldNormal = (inst.normalMatrix * float4(position, 0.0)).xyz;
    
    // Transform to clip space: Projection * View * World
    out.position = uniforms.projectionMatrix * uniforms.viewMatrix * worldPos;
    
    out.worldPos = worldPos.xyz;
    out.normal = normal;
    out.texcoord = in_packed.texcoord;
    out.instanceID = instanceID;
    
    return out;
    
}

// -- Fragment Shader --

fragment float4 fragmentMain (
    VertexOut in [[stage_in]],
    constant InstanceData* instances [[buffer(BufferIndexInstanceData)]]
) {
    uint64_t addr = instances[in.instanceID].materialAddress;
    
    device const MaterialArguments &material = *(device const MaterialArguments*)addr;
    
    float4 texColor = material.baseColorTexture.sample(textureSampler, in.texcoord);
    
    float3 lightDir = normalize(float3(1.0, 1.0, 1.0));
    float NdotL = max(dot(normalize(in.normal), lightDir), 0.0);
    float3 diffuse = texColor.rgb * (0.3 + 0.7 * NdotL);
    
    return float4(diffuse, texColor.a);
}
