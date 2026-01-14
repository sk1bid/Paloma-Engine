//
//  SpheresScene.hpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#pragma once

#include "Mesh.hpp"
#include "Metal/Metal.hpp"
#include "Scene.hpp"
#include "ShaderTypes.h"

namespace Paloma {

class SpheresScene : public Scene {
public:
    SpheresScene() = default;
    ~SpheresScene() override;
    
    // -- Scene Interface --
    void setup(Renderer *renderer) override;
    void cleanup() override;
    void update(float dt) override;
    void render(Renderer *renderer) override;
    
private:
    static constexpr uint32_t kInstanceCount = 5;
    Renderer* _renderer;
    
    // Resources
    Mesh *_sphereMesh = nullptr;
    
    
    uint64_t _onyxMatAddress = 0;
    uint64_t _fabricMatAddress = 0;

    MTL::Buffer *_instanceBuffers[MaxFramesInFlight] = {};
    
    float _rotation = 0.0f;
    
    // Instance data
    InstanceData _instances[kInstanceCount];
    
    MTL::Buffer* _environmentBuffer;
};
} // namespace Paloma
