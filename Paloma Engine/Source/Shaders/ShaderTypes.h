//
//  ShaderTypes.h
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#ifndef ShaderTypes_h
#define ShaderTypes_h

#include <simd/simd.h>

#ifndef __METAL__
#include <Metal/MTLTypes.hpp>
#endif

// -- buffer indices for argument table --
enum BufferIndex {
    BufferIndexUniforms = 0,  // frame uniforms
    BufferIndexVertices = 1,  // vertex data
    BufferIndexInstanceData = 2,  // Instance transforms
    BufferIndexMaterial = 3,
};

// -- texture indices for argument table --
enum TextureIndex {
    TextureIndexColor = 0, // albedo/diffuse color map
    TextureIndexNormal = 1, // Normal map
    TextureIndexMetallic = 2, // Metallic/Roughness map
};

// -- Vertex attributes (vertex arreibutes) --
enum VertexAttribute {
    VertexAttributePosition = 0,
    VertexAttributeNormal = 1,
    VertexAttributeTexcoord = 2,
};

// Spherical Harmonics L2
struct SHCoefficients {
    simd_float3 L00;
    simd_float3 L1m1;
    simd_float3 L10;
    simd_float3 L11;
    simd_float3 L2m2;
    simd_float3 L2m1;
    simd_float3 L20;
    simd_float3 L21;
    simd_float3 L22;
};

// -- Frame uniforms for every frame --
struct FrameUniforms {
    simd_float4x4 viewMatrix;       // Camera
    simd_float4x4 projectionMatrix; // Projection
    
    simd_float4x4 viewMatrixInverse;
    
    simd_float4x4 projectionMatrixInverse;
    simd_float3 cameraPosition;
    float time; // for animation
    float iblIntensity;
    float exposure;
    uint64_t environmentAddress;
    SHCoefficients sh;
};

// -- instance data --
struct InstanceData {
    simd_float4x4 modelMatrix; // world transform
    simd_float4x4 normalMatrix;
    uint64_t materialAddress;
};

// -- Vertex for procedural mesh ---
struct Vertex {
    simd_float4 position;
    simd_float3 normal;
    simd_float4 tangent;
    simd_float2 texcoord;
};

struct EnvironmentData {
#ifdef __METAL__
    metal::texturecube<float> skyboxMap;
    metal::texturecube<float> prefilteredMap;
    metal::texture2d<float> brdfLut;
#else
    MTL::ResourceID skyboxMapID;
    MTL::ResourceID prefilteredMapID;
    MTL::ResourceID brdfLutID;
#endif
};
typedef struct {
    simd_float4 baseColorFactor;
    simd_float4 emissiveFactor;
    float metallicFactor;
    float roughnessFactor;
    float normalScale;
    float occlusionStrength;
    float opacityFactor;
    float alphaCutoff;
    
    uint32_t hasBaseColorTexture;
    uint32_t hasNormalTexture;
    uint32_t hasRoughnessTexture;
    uint32_t hasMetalnessTexture;
} MaterialConstants;

typedef struct {
    MaterialConstants constants;
    
#ifdef __METAL__
    metal::texture2d<float> baseColorTexture;
    metal::texture2d<float> normalTexture;
    metal::texture2d<float> emissiveTexture;
    metal::texture2d<float> occlusionTexture;
    metal::texture2d<float> metalnessTexture;
    metal::texture2d<float> roughnessTexture;
    metal::texture2d<float> opacityTexture;
#else
    MTL::ResourceID baseColorTexture;
    MTL::ResourceID normalTexture;
    MTL::ResourceID emissiveTexture;
    MTL::ResourceID occlusionTexture;
    MTL::ResourceID metalnessTexture;
    MTL::ResourceID roughnessTexture;
    MTL::ResourceID opacityTexture;
#endif
} MaterialArguments;


#endif
