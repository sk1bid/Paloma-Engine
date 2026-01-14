//
//  Renderer.hpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#pragma once

#include "Types.hpp"
#include "Context.hpp"
#include "AssetManager.hpp"
#include "PipelineCache.hpp"
#include "ShaderTypes.h"

// Forward declarations
namespace MTL{
class Device;
class Texture;
class DepthStencilState;
class Buffer;
}

namespace MTL4 {
class RenderPassDescriptor;
class RenderCommandEncoder;
}
namespace CA {
class MetalDrawable;
}

namespace Paloma {

class Scene;

class Renderer {
public:
    Renderer() = default;
    ~Renderer();
    
    // Copy is forbbiden
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    
    // -- Lifecycle --
    bool init(MTL::Device* device);
    void shutdown();
    
    // --Frame--
    void beginFrame();
    void endFrame(CA::MetalDrawable* drawable, MTL::ResidencySet* layerResidency);
    
    // --Render--
    void renderSkybox();
    void render(MTL4::RenderPassDescriptor* renderPass,
                CA::MetalDrawable* drawable,
                MTL::ResidencySet* layerResidency,
                float width, float height, float dt);
    
    
    // -- Scene --
    void setScene(Scene* scene);
    
    // -- Accessors --
    Context* context() {return &_context;}
    AssetManager* assetManager() { return &_assetManager;}
    float totalTime() {return _totalTime;}
    MTL4::RenderCommandEncoder* currentEncoder() { return _currentEncoder; }
    MTL4::ArgumentTable* argumentTable();
    
private:
    // -- Core --
    Context _context;
    AssetManager _assetManager;
    PipelineCache _pipelineCache;
    
    // --Render state--
    MTL::DepthStencilState* _depthState = nullptr;
    MTL::DepthStencilState* _skyboxDepthState = nullptr;
    
    MTL::Buffer* _uniformBuffers[MaxFramesInFlight] = {};
    MTL::Buffer* _environmentBuffer = nullptr;
    
    // --Current frame --
    MTL4::RenderCommandEncoder* _currentEncoder = nullptr;
    Scene* _currentScene = nullptr;
    
    float _aspectRatio = 1.0f;
    
    float _totalTime = 0.0; // Global time for animation
    
};
}
