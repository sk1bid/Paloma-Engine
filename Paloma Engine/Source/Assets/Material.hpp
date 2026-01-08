//
//  Material.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include <Metal/Metal.hpp>
#include <simd/simd.h>
#include <string>

namespace Paloma {

// -- Material - PBR material --
struct Material {
    std::string name;
    
    // Pipeline state for this material
    MTL::RenderPipelineState* pipeline = nullptr;
    
    // -- Textures (ResourceID for ArgumentTable) --
    MTL::ResourceID albedoMap = {0}; // Color
    MTL::ResourceID normalMap = {0}; // Normals
    MTL::ResourceID metallicMap = {0}; // Metallic/Roughness
    
    // -- Options if not texture --
    simd::float4 albedoColor = {1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f; // diellectric
    float roughness = 0.5f; // middle rough
    
    // -- flags --
    bool hasAlbedoMap = false;
    bool hasNormalMap = false;
    bool hasMetallicMap = false;
    
    // -- Utils --
    
    // set albedo
    void setAlbedoTexture(MTL::Texture* texture){
        if (texture){
            albedoMap = texture->gpuResourceID();
            hasAlbedoMap = true;
        }
    }
    
    // set normal texture
    void setNormalTexture(MTL::Texture* texture) {
        if (texture) {
            normalMap = texture->gpuResourceID();
            hasNormalMap = true;
        }
    }
    
    void setMetallicTexture(MTL::Texture* texture) {
        if (texture){
            metallicMap = texture->gpuResourceID();
            hasMetallicMap = true;
        }
    }
    
    
};
}
