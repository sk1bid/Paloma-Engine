//
//  SpheresScene.cpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include "SpheresScene.hpp"
#include "Renderer.hpp"
#include "AAPLMathUtilities.h"
#include "Bridge.hpp"

namespace Paloma {

SpheresScene::~SpheresScene() {
    cleanup();
}

void SpheresScene::cleanup(){
    for (uint32_t i = 0; i < MaxFramesInFlight; i++){
        if (_instanceBuffers[i]){
            _instanceBuffers[i]->release();
            _instanceBuffers[i] = nullptr;
        }
    }
    
    _sphereMesh = nullptr;
    _texture = nullptr;
}

// -- Setup Scene (once) --
void SpheresScene::setup(Renderer* renderer){
    auto device = renderer->context()->device();
    auto assetManager = renderer->assetManager();
    
    // Load sphere mesh
    _sphereMesh = assetManager->getPrimitive("sphere");
    
    // Load texture
    std::string texPath = Bridge::getBundleResourcePath(
                                                        "storage-container2-albedo", "png"
                                                        );
    if (texPath.empty()) {
        printf("ERROR: Texture not found in bundle!\n");
    }else if (!texPath.empty()) {
        _texture = assetManager->getTexture(texPath.c_str(), true);
    }
    
    if (_texture == nullptr) {
        printf("WARNING: No texture bound!\n");
        return;
    }
    
    
    // Create instance buffers
    for (uint32_t i = 0; i < MaxFramesInFlight; i++){
        _instanceBuffers[i] = device->newBuffer(sizeof(InstanceData) * kInstanceCount,
                                                MTL::ResourceStorageModeShared);
    }
    
    // Register in residency
    auto residency = renderer->context()->residencySet();
    for (uint32_t i = 0; i < MaxFramesInFlight; i++){
        residency->addAllocation(_instanceBuffers[i]);
    }
    assetManager->registerWithResidencySet(residency);
    
    // Setup camera
    _camera.position = {0.0f, 0.0f, 5.0f};
    _camera.target = {0.0f, 0.0f, 0.0f};
}

// Update (every frame)
void SpheresScene::update(float dt){
    _rotation+=dt;
    
    for (uint32_t i = 0; i < kInstanceCount; i++){
        float xPos = (float)i * 1.2 - (float)(kInstanceCount - 1) / 2.0f;
        
        float angle = _rotation * (1.0f + i * 0.2f);
        
        simd_float4x4 rotationY = matrix4x4_rotation(angle, 0.0f, 1.0f, 0.0f);
        simd_float4x4 rotationX = matrix4x4_rotation(angle * 0.5f, 1.0f, 0.0f, 0.0f);
        // Build model matrix
        simd_float4x4 translation = matrix4x4_translation(xPos, 0.0f, 0.0f);
        simd_float4x4 rotationMat = simd_mul(rotationY, rotationX);
        
        _instances[i].modelMatrix = simd_mul(translation, rotationMat);
        _instances[i].normalMatrix = _instances[i].modelMatrix;
    }
}

// Render (every frame) --

void SpheresScene::render(Renderer *renderer) {
    if (!_sphereMesh) return;
    
    auto encoder = renderer->currentEncoder();
    auto argTable = renderer->argumentTable();
    uint32_t frameIndex = renderer->context()->frameIndex();
    
    // Copy instance data to GPU buffer
    memcpy(_instanceBuffers[frameIndex]->contents(),
           _instances,
           sizeof(InstanceData) * kInstanceCount);
    
    // Bind instance buffers
    argTable->setAddress(_instanceBuffers[frameIndex]->gpuAddress(),
                         BufferIndexInstanceData);
    
    // Bind vertex buffer
    argTable->setAddress(_sphereMesh->vertexAddress(), BufferIndexVertices);
    
    // Bind texture
    if (_texture) {
        argTable->setTexture(_texture->gpuResourceID(), TextureIndexColor);
    }
    
    // draw all instances
    for (const auto& submesh : _sphereMesh->submeshes()){
        encoder->drawIndexedPrimitives(MTL::PrimitiveTypeTriangle,
                                       submesh.indexCount,
                                       submesh.indexType,
                                       _sphereMesh->indexAdress(),
                                       _sphereMesh->indexBuffer()->length(),
                                       kInstanceCount);
    }
}
}
