//
//  Material.hpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#pragma once
#include <Metal/Metal.hpp>
#include <simd/simd.h>
#include "BufferUtilites.hpp"

enum class AlphaMode : uint32_t {
    Opaque = 0,
    Mask = 1,
    Blend = 2
};

struct ScalarProperty {
    NS::SharedPtr<MTL::Texture> pTexture;
    float factor = 1.0f;
    int mappingChannel = 0;
};

struct ColorProperty {
    NS::SharedPtr<MTL::Texture> pTexture;
    simd_float3 factor = { 0, 0, 0 };
    int mappingChannel = 0;
};

class Material {
public:
    NS::SharedPtr<NS::String> name;

    ColorProperty  baseColor;
    ScalarProperty opacity;
    ScalarProperty metalness;
    ScalarProperty roughness;
    ColorProperty  emissive;
    ScalarProperty normal;
    ScalarProperty occlusion;

    AlphaMode alphaMode = AlphaMode::Opaque;
    float alphaThreshold = 0.5f;

    int relativeSortOrder() const {
        switch (alphaMode) {
            case AlphaMode::Opaque: return -1;
            case AlphaMode::Mask:   return 0;
            case AlphaMode::Blend:  return 1;
        }
        return -1;
    }

    NS::SharedPtr<MTL::RenderPipelineState> pRenderPipelineState;
    
    BufferView bufferView = { nullptr, 0, 0 };

public:
    Material() = default;
};
