//
//  AssetManager.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include "Mesh.hpp"
#include <Metal/Metal.hpp>
#include "ShaderTypes.h"
#include <unordered_map>
#include "string"

namespace Paloma {

static constexpr size_t MaxMaterials = 1024;
static constexpr size_t MaterialBufferSize = MaxMaterials * sizeof(MaterialArguments);

struct TextureResource {
    MTL::Texture* texture;
    uint32_t bindlessIndex;
};

// -- AssetManager - central cache for resources --
class AssetManager {
public:
    /// Init
    void init(MTL::Device* device, MTL4::ArgumentTable* argTable);
    
    /// Clear all resources
    void shutdown();
    
    // -- Mesh --
    
    /// Get Mesh
    Mesh* getMesh(const char* path);
    
    /// Get Primitive (box, sphere, plane)
    Mesh* getPrimitive(const char* name);
    
    // -- Textures --
    
    /// Get Texture (load if not in cache)
    TextureResource getTexture(const char* path, bool sRGB = true);
    
    /// Get Texture from bundle
    TextureResource getBundleTexture(const char* name, bool sRGB = true);
    
    /// Get Texture from .hdr format
    TextureResource getHDRTexture(const char* path);
    
    /// Get Texture index
    uint32_t getTextureIndex(const char* path);
    
    // -- Materials --
    uint64_t getMaterialGPUAddress(const std::string& materialName);
    
    uint64_t createMaterial(const std::string& name, const MaterialArguments& args);
    
    // -- ResidencySet --
    
    /// Add all resources to residency set
    void registerWithResidencySet(MTL::ResidencySet* residencySet);
    
private:
    MTL::Device* _device = nullptr;
    MTL4::ArgumentTable* _argTable = nullptr;
    std::unordered_map<std::string, Mesh*> _meshes;
    std::unordered_map<std::string, TextureResource> _textures;
    MTL::Buffer* _materialBuffer = nullptr;
    size_t _materialCounter = 0;
    std::unordered_map<std::string, uint64_t> _materialOffsets;
    uint32_t _nextTextureIndex = 0;
};
}
