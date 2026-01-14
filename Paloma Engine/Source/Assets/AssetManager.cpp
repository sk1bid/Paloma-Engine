//
//  AssetManager.cpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#include "AssetManager.hpp"
#include "MeshLoader.hpp"
#include "TextureLoader.hpp"

namespace Paloma {

// -- Init / shutdown --

void AssetManager::init(MTL::Device *device, MTL4::ArgumentTable *argTable){
    _device = device;
    _argTable = argTable;
    _nextTextureIndex = 0;
    
    _materialBuffer = _device->newBuffer(MaterialBufferSize, MTL::ResourceStorageModeShared);
}

void AssetManager::shutdown(){
    // Clear all mesh
    for (auto& [path, mesh] : _meshes) {
        delete mesh;
    }
    _meshes.clear();
    
    // Clear all textures
    for (auto& [path, resource] : _textures){
        if (resource.texture){
            resource.texture->release();
        }
    }
    _textures.clear();
    _materialBuffer->release();
    
    _device = nullptr;
}

// -- Mesh --

Mesh* AssetManager::getMesh(const char *path){
    // Check cache
    auto it = _meshes.find(path);
    /// cache hit
    if (it != _meshes.end()){
        return it->second;
    }
    
    // Load
    Mesh* mesh = MeshLoader::loadFromFile(_device, path);
    if (mesh) {
        _meshes[path] = mesh;
    }
    return mesh;
}

Mesh* AssetManager::getPrimitive(const char* name) {
    std::string key = std::string("primitive:") + name;
    
    auto it = _meshes.find(key);
    /// cache hit
    if (it != _meshes.end()){
        return it->second;
    }
    
    Mesh* mesh = MeshLoader::loadPrimitive(_device, name);
    if (mesh) {
        _meshes[key] = mesh;
    }
    return mesh;
}

// -- Textures --

TextureResource AssetManager::getTexture(const char* path, bool sRGB) {
    auto it = _textures.find(path);
    /// cache hit
    if (it!=_textures.end()){
        return it->second;
    }
    
    MTL::Texture* texture = TextureLoader::loadFromFile(_device, path, sRGB);
    if (texture) {
        uint32_t index = _nextTextureIndex++;
        TextureResource res = { texture, index };
        _textures[path] = { texture, index };
        
        _argTable->setTexture(texture->gpuResourceID(), index);
        return res;
    }
    return {nullptr, 0};
}

TextureResource AssetManager::getBundleTexture(const char* name, bool sRGB) {
    std::string key = std::string("bundle:") + name;
    
    auto it = _textures.find(key);
    if (it != _textures.end()) {
        return it->second;
    }
    
    MTL::Texture* texture = TextureLoader::loadFromFile(_device, name, sRGB);
    if (texture) {
        uint32_t index = _nextTextureIndex++;
        TextureResource res = { texture, index };
        _textures[key] = { texture, index };
        
        _argTable->setTexture(texture->gpuResourceID(), index);
        return res;
    }
    return {nullptr, 0};
}

TextureResource AssetManager::getHDRTexture(const char* path){
    std::string key(path);
    
    // check cache
    auto it = _textures.find(key);
    if (it != _textures.end()){
        return it->second;
    }
    
    MTL::Texture* texture = TextureLoader::loadHDR(_device, path);
    if (!texture){
        return {nullptr, 0};
    }
    
    // register in arg table
    uint32_t index = _nextTextureIndex++;
    _argTable->setTexture(texture->gpuResourceID(), index);
    
    TextureResource resource = {texture, index};
    _textures[key] = resource;
    return resource;
}


uint32_t AssetManager::getTextureIndex(const char* path) {
    auto it = _textures.find(path);
    if (it != _textures.end()) {
        return it->second.bindlessIndex;
    }
    return 0;
}

// -- Materials --

uint64_t AssetManager::createMaterial(const std::string& name, const MaterialArguments& args){
    if (_materialCounter + 1 > MaxMaterials) return 0;
    
    auto im = _materialOffsets.find(name);
    if (im != _materialOffsets.end()) {
        return im->second;
    }
    
    uint64_t offset = _materialCounter * sizeof(MaterialArguments);
    
    void* dataPtr = (uint8_t*)_materialBuffer->contents() + offset;
    memcpy(dataPtr, &args, sizeof(MaterialArguments));
    
    _materialOffsets[name] = offset;
    
    uint64_t gpuAddr = _materialBuffer->gpuAddress() + offset;
    
    _materialCounter++;
    return gpuAddr;
}

// -- residency set --

void AssetManager::registerWithResidencySet(MTL::ResidencySet *residencySet){
    /// add all mesh buffers
    for (auto& [path, mesh] : _meshes) {
        if (mesh) {
            residencySet->addAllocation(mesh->vertexBuffer());
            residencySet->addAllocation(mesh->indexBuffer());
        }
    }
    
    /// add all textures
    for (auto& [path, resource] : _textures){
        if (resource.texture){
            residencySet->addAllocation(resource.texture);
        }
    }
    
    residencySet->addAllocation(_materialBuffer);
    
    residencySet->commit();
}

}
