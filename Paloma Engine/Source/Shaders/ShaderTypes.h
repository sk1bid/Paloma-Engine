//
//  ShaderTypes.h
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

// -- buffer indices for argument table --
enum BufferIndex {
    BufferIndexUniforms = 0,  // frame uniforms
    BufferIndexVertices = 1,  // vertex data
    BufferIndexInstanceData = 2,  // Instance transforms
};

// -- Vertex attributes (vertex arreibutes) --
enum VertexAttribute {
    VertexAttributePosition = 0,
    VertexAttributeNormal = 1,
    VertexAttributeTexcoord = 2,
};

// -- Frame uniforms for every frame --
struct FrameUniforms {
    simd_float4x4 viewMatrix;       // Camera
    simd_float4x4 projectionMatrix; // Projection
    simd_float3 cameraPosition;
    float time; // for animation
};

// -- instance data --
struct InstanceData {
    simd_float4x4 modelMatrix; // world transform
    simd_float4x4 normalMatrix;
};

// -- Vertex for procedural mesh ---
struct Vertex {
    simd_float3 position;
    simd_float3 normal;
    simd_float2 texcoord;
};

#endif
