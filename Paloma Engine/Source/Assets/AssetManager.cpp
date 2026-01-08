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

void AssetManager::init(MTL::Device *device){
    _device = device;
}

void AssetManager::shutdown(){
    // Clear all mesh
    for (auto& [path, mesh] : _meshes) {
        delete mesh;
    }
    _meshes.clear();
    
    // Clear all textures
    for (auto& [path, texture] : _textures){
        if (texture){
            texture->release();
        }
    }
    _textures.clear();
    
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

MTL::Texture* AssetManager::getTexture(const char* path, bool sRGB) {
    auto it = _textures.find(path);
    /// cache hit
    if (it!=_textures.end()){
        return it->second;
    }
    
    MTL::Texture* texture = TextureLoader::loadFromFile(_device, path, sRGB);
    if (texture) {
        _textures[path] = texture;
    }
    return texture;
}

MTL::Texture* AssetManager::getBundleTexture(const char* name, bool sRGB) {
    std::string key = std::string("bundle:") + name;
    
    auto it = _textures.find(key);
    if (it != _textures.end()) {
        return it->second;
    }
    
    MTL::Texture* texture = TextureLoader::loadFromBundle(_device, name, sRGB);
    if (texture) {
        _textures[key] = texture;
    }
    return texture;
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
    for (auto& [path, texture] : _textures){
        if (texture){
            residencySet->addAllocation(texture);
        }
    }
    
    residencySet->commit();
}

}
