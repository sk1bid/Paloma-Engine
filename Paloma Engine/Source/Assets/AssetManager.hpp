//
//  AssetManager.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include "Mesh.hpp"
#include <Metal/Metal.hpp>
#include <unordered_map>
#include "string"

namespace Paloma {

// -- AssetManager - central cache for resources --
class AssetManager {
public:
    /// Init
    void init(MTL::Device* device);
    
    /// Clear all resources
    void shutdown();
    
    // -- Mesh --
    
    /// Get Mesh
    Mesh* getMesh(const char* path);
    
    /// Get Primitive (box, sphere, plane)
    Mesh* getPrimitive(const char* name);
    
    // -- Textures --
    
    /// Get Texture (load if not in cache)
    MTL::Texture* getTexture(const char* path, bool sRGB = true);
    
    /// Get Texture from bundle
    MTL::Texture* getBundleTexture(const char* name, bool sRGB = true);
    
    // -- ResidencySet --
    
    /// Add all resources to residency set
    void registerWithResidencySet(MTL::ResidencySet* residencySet);
    
private:
    MTL::Device* _device = nullptr;
    std::unordered_map<std::string, Mesh*> _meshes;
    std::unordered_map<std::string, MTL::Texture*> _textures;
        
};
}
