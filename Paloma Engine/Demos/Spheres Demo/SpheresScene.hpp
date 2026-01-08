//
//  SpheresScene.hpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#pragma once

#include "Scene.hpp"
#include "ShaderTypes.h"
#include "Mesh.hpp"
#include "Metal/Metal.hpp"

namespace Paloma {

class SpheresScene : public Scene {
public:
    SpheresScene() = default;
    ~SpheresScene() override;
    
    // -- Scene Interface --
    void setup(Renderer* renderer) override;
    void cleanup() override;
    void update(float dt) override;
    void render(Renderer* renderer) override;
    
private:
    static constexpr uint32_t kInstanceCount = 5;
    
    // Resources
    Mesh* _sphereMesh = nullptr;
    MTL::Texture* _texture = nullptr;
    MTL::Buffer* _instanceBuffers[MaxFramesInFlight] = {};
    
    float _rotation = 0.0f;
    
    // Instance data
    InstanceData _instances[kInstanceCount];
};
}

