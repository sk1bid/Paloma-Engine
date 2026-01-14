//
//  Renderer.cpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include "Renderer.hpp"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "Scene.hpp"
#include "Camera.hpp"

namespace Paloma {

// --Destructor--
Renderer::~Renderer() {
    shutdown();
}

// --Init--
bool Renderer::init(MTL::Device *device) {
    // init context
    if (!_context.init(device)){
        return false;
    }
    
    // Pipeline Cache init
    _pipelineCache.init(_context.device(), _context.compiler());
    
    // Asset Manager init
    _assetManager.init(_context.device(), _context.argumentTable());
    
    // Uniform buffers
    for (uint32_t i=0; i < MaxFramesInFlight; i++){
        _uniformBuffers[i] = _context
            .device()->newBuffer(sizeof(FrameUniforms),
                                 MTL::ResourceStorageModeShared);
        
        _uniformBuffers[i]->setLabel(
                                     NS::String::string("UniformBuffer", NS::UTF8StringEncoding)
                                     );
    }
    
    auto residency = _context.residencySet();
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        residency->addAllocation(_uniformBuffers[i]);
    }
    _environmentBuffer = _context.device()->newBuffer(
        sizeof(EnvironmentData),
        MTL::ResourceStorageModeShared
    );
    _environmentBuffer->setLabel(NS::String::string("EnvironmentBuffer", NS::UTF8StringEncoding));
    residency->addAllocation(_environmentBuffer);
    residency->commit();
    
    // Depth state
    auto depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
    depthDesc->setDepthWriteEnabled(true);
    _depthState = _context.device()->newDepthStencilState(depthDesc);
    depthDesc->release();
    
    // Skybox depth state
    auto skyboxDepthDesc = MTL::DepthStencilDescriptor::alloc()->init();
    skyboxDepthDesc->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
    skyboxDepthDesc->setDepthWriteEnabled(false);
    _skyboxDepthState = _context.device()->newDepthStencilState(skyboxDepthDesc);
    skyboxDepthDesc->release();
    
   
    return true;
}

// -- Shutdown --
void Renderer::shutdown() {
    if (_environmentBuffer) {
        _environmentBuffer->release();
        _environmentBuffer = nullptr;
    }
    if (_skyboxDepthState) {
        _skyboxDepthState->release();
        _skyboxDepthState = nullptr;
    }
    if (_depthState) {
        _depthState->release();
        _depthState = nullptr;
    }
    
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        if (_uniformBuffers[i]) {
            _uniformBuffers[i]->release();
            _uniformBuffers[i] = nullptr;
        }
    }
    
    _assetManager.shutdown();
    _pipelineCache.shutdown();
    _context.shutdown();
}

// --Frame Management --
void Renderer::beginFrame(){
    _context.beginFrame();
}

void Renderer::endFrame(CA::MetalDrawable *drawable, MTL::ResidencySet *layerResidency) {
    _context.endFrame(drawable, layerResidency);
}

// --Scene --
void Renderer::setScene(Scene *scene){
    _currentScene = scene;
}

// -- Accessor --
MTL4::ArgumentTable* Renderer::argumentTable() {
    return _context.argumentTable();
}

void Renderer::renderSkybox() {
    auto skyboxPipeline = _pipelineCache.getSkybox();
    if (!skyboxPipeline) return;
    
    _currentEncoder->setRenderPipelineState(skyboxPipeline);
    
    _currentEncoder->setDepthStencilState(_skyboxDepthState);
    

    _currentEncoder->setCullMode(MTL::CullModeNone);
    
    
    _currentEncoder->drawPrimitives(MTL::PrimitiveTypeTriangle,
                                     NS::UInteger(0),
                                     NS::UInteger(3));
    
    _currentEncoder->setDepthStencilState(_depthState);
}

// --Render--
void Renderer::render(MTL4::RenderPassDescriptor *renderPass, CA::MetalDrawable *drawable, MTL::ResidencySet *layerResidency, float width, float height, float dt){
    _totalTime+=dt;
    _aspectRatio = width / height;
    
    // Begin frame
    beginFrame();
    
    // Create encoder
    _currentEncoder = _context.commandBuffer()->renderCommandEncoder(renderPass);
    if (!_currentEncoder){
        return;
    }
    
    auto pipeline = _pipelineCache.getDefault();
    if (pipeline) {
        _currentEncoder->setRenderPipelineState(pipeline);
        _currentEncoder->setDepthStencilState(_depthState);
        _currentEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        _currentEncoder->setCullMode(MTL::CullModeBack);
    }
    
    // Bind argument table
    _currentEncoder->setArgumentTable(_context.argumentTable(),
                                      MTL::RenderStageVertex | MTL::RenderStageFragment);
    
    // Update uniforms
    uint32_t frameIndex = _context.frameIndex();
    FrameUniforms uniforms = {};
    
    if (_currentScene){
        const Camera& cam = _currentScene->camera();
        uniforms.viewMatrix = cam.viewMatrix();
        uniforms.projectionMatrix = cam.projectionMatrix();
        uniforms.cameraPosition = cam.position;
        uniforms.viewMatrixInverse = simd_inverse(cam.viewMatrix());
        uniforms.projectionMatrixInverse = simd_inverse(cam.projectionMatrix());
        if (_currentScene->skyboxTexture()) {
            EnvironmentData* envData = (EnvironmentData*)_environmentBuffer->contents();
            envData->skyboxTextureID = _currentScene->skyboxTexture()->gpuResourceID();
            
            uniforms.environmentAddress = _environmentBuffer->gpuAddress();
        }
    }
    
    uniforms.time = _totalTime;
    
    memcpy(_uniformBuffers[frameIndex]->contents(), &uniforms, sizeof(FrameUniforms));
    _context.argumentTable()->setAddress(
                                         _uniformBuffers[frameIndex]->gpuAddress(),
                                         BufferIndexUniforms
                                         );
    
    // Render scene
    if (_currentScene) {
        _currentScene->render(this);
        
        if (_currentScene->skyboxTexture()){
            renderSkybox();
        }
    }
    
    // End
    _currentEncoder->endEncoding();
    _currentEncoder = nullptr;
    
    endFrame(drawable, layerResidency);
}
}
