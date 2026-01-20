//
//  Scene.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#pragma once
#include <Metal/Metal.hpp>
#include <ModelIO/ModelIO.hpp>
#include <map>
#include <string>
#include <vector>
// #include "Entity.hpp"
#include "ImageBasedLight.hpp"
#include "ShaderStructures.h"

class Entity;
class Scene {
public:
public:
    static Scene *load(const std::string &path, MTL::Device *pDevice);
    const std::vector<NS::SharedPtr<MTL::Resource>>& getResources() const { return resources; }
    ImageBasedLight* getLightingEnvironment() const { return pLightingEnvironment; }
private:
    friend class Metal4Renderer;
    std::shared_ptr<Entity> rootEntity;
    
    std::vector<Light> lights;
    
    ImageBasedLight *pLightingEnvironment = nullptr;
    
    std::vector<NS::SharedPtr<MTL::Resource>> resources;
    
    
    
    Scene() = default;
};
