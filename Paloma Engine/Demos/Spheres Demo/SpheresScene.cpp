//
//  SpheresScene.cpp
//  Paloma Engine
//
//  Created by Artem on 08.01.2026.
//

#include "SpheresScene.hpp"
#include "AAPLMathUtilities.h"
#include "Bridge.hpp"
#include "Components.hpp"
#include "Renderer.hpp"

namespace Paloma {

SpheresScene::~SpheresScene() { cleanup(); }

void SpheresScene::cleanup() {
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        if (_instanceBuffers[i]) {
            _instanceBuffers[i]->release();
            _instanceBuffers[i] = nullptr;
        }
    }
    
    _monkeyMesh = nullptr;
}

// -- Setup Scene (once) --
void SpheresScene::setup(Renderer *renderer) {
    _renderer = renderer;
    auto device = _renderer->context()->device();
    auto assetManager = _renderer->assetManager();
    _environmentBuffer = device->newBuffer(sizeof(EnvironmentData), MTL::ResourceStorageModeShared);
    // Load sphere mesh
//    std::string monkeyPath = Bridge::getBundleResourcePath("monkey", "obj");
//    _monkeyMesh = assetManager->getMesh(monkeyPath.c_str());
    
    _monkeyMesh = assetManager->getPrimitive("sphere");
    
    
    // Load skybox hdr texture
    std::string pathSkyboxHDR = Bridge::getBundleResourcePath("kloppenheim_06_4k", "hdr");
    //std::string pathSkyboxHDR = Bridge::getBundleResourcePath("Beach", "hdr");
    auto hdrRes = assetManager->getHDRTexture(pathSkyboxHDR.c_str());
    
    auto iblAllocator = device->newCommandAllocator();
    _iblResource = IBLGenerator::generate(
                                          device,
                                          renderer->context()->commandQueue(),
                                          iblAllocator,
                                          hdrRes.texture,
                                          512
                                          );
    iblAllocator->release();
    _skyboxTexture = _iblResource.environmentCubemap;
    
    //    // Add metal mat
    //    MaterialArguments metalMat = {};
    //    std::string pathMetalColor = Bridge::getBundleResourcePath("storage-container2-albedo", "png");
    //    std::string pathMetalNormal = Bridge::getBundleResourcePath("storage-container2-normal-ogl", "png");
    //    std::string pathMetalRoughness = Bridge::getBundleResourcePath("storage-container2-roughness", "png");
    //    std::string pathMetalMetalness = Bridge::getBundleResourcePath("storage-container2-metallic", "png");
    //
    //    auto texMetalColor = assetManager->getTexture(pathMetalColor.c_str(), true);
    //    auto texMetalNormal = assetManager->getTexture(pathMetalNormal.c_str(), false);
    //    auto texMetalRoughness = assetManager->getTexture(pathMetalRoughness.c_str(),false);
    //    auto texMetalMetalness = assetManager->getTexture(pathMetalMetalness.c_str(), false);
    //
    //    metalMat.baseColorTexture = texMetalColor.texture->gpuResourceID();
    //    metalMat.normalTexture = texMetalNormal.texture->gpuResourceID();
    //    metalMat.roughnessTexture = texMetalRoughness.texture->gpuResourceID();
    //    metalMat.metalnessTexture = texMetalMetalness.texture->gpuResourceID();
    //
    //    MaterialConstants metalParams = {};
    //    metalParams.metallicFactor = 1;
    //    metalParams.roughnessFactor = 1;
    //    metalMat.constants = metalParams;
    //
    //    _metalMatAddress = assetManager->createMaterial("Metal", metalMat);
    //
    // Add fabric material
    MaterialArguments fabricMat = {};
    std::string texPathFabric = Bridge::getBundleResourcePath("Fabric080_2K-PNG_Color", "png");
    std::string pathFabricNormal = Bridge::getBundleResourcePath("Fabric080_2K-PNG_NormalGL", "png");
    std::string pathFabricRoughness = Bridge::getBundleResourcePath("Fabric080_2K-PNG_Roughness", "png");
    
    auto textureFabric = assetManager->getTexture(texPathFabric.c_str(), true);
    auto texFabricNormal = assetManager->getTexture(pathFabricNormal.c_str(), false);
    auto texFabricRoughness = assetManager->getTexture(pathFabricRoughness.c_str(),
                                                       false);
    MaterialConstants fabricParams = {};
    fabricParams.baseColorFactor = simd_float4{1, 1, 1, 1};
    fabricParams.metallicFactor = 0.0;
    fabricParams.roughnessFactor = 1.0;
    fabricParams.hasNormalTexture = 1;
    fabricParams.hasBaseColorTexture = 1;
    fabricParams.hasRoughnessTexture = 1;
    fabricParams.hasMetalnessTexture = 0;
    fabricMat.constants = fabricParams;
    
    fabricMat.baseColorTexture = textureFabric.texture->gpuResourceID();
    fabricMat.normalTexture = texFabricNormal.texture->gpuResourceID();
    fabricMat.roughnessTexture = texFabricRoughness.texture->gpuResourceID();
    
    _fabricMatAddress = assetManager->createMaterial("Fabric", fabricMat);
    
    MaterialArguments simpleMat = {};
    MaterialConstants simpleConstants = {};
    simpleConstants.baseColorFactor = simd_float4{1,1,1,1};
    simpleConstants.metallicFactor = 1.0;
    simpleConstants.roughnessFactor = 0.05;
    simpleConstants.hasBaseColorTexture = 0;
    simpleConstants.hasNormalTexture = 0;
    simpleConstants.hasRoughnessTexture = 0;
    simpleConstants.hasMetalnessTexture = 0;
    simpleMat.constants = simpleConstants;
    _simpleMatAddress  = assetManager->createMaterial("Simple", simpleMat);
    
    
    for (int i = 0; i < kInstanceCount; i++) {
        auto entity = _registry.create();
        
        float x = (i - 2) * 2.0f;
        _registry.emplace<TransformComponent>(entity, vector_float3{x, 0, 0}, // pos
                                              vector_float3{0, 0, 0},// rot
                                              vector_float3{1, 1, 1} // scale
                                              );
        
        
        _registry.emplace<MeshComponent>(entity,
                                         _monkeyMesh,
                                         _simpleMatAddress);
        
        
    }
    
    // Create instance buffers
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        _instanceBuffers[i] = device->newBuffer(
                                                sizeof(InstanceData) * kInstanceCount, MTL::ResourceStorageModeShared);
    }
    
    // Register in residency
    auto residency = renderer->context()->residencySet();
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        residency->addAllocation(_instanceBuffers[i]);
    }
    if (_iblResource.readyEvent) {
        renderer->context()->commandQueue()->wait(_iblResource.readyEvent, 1);
        _iblResource.readyEvent->release();
        _iblResource.readyEvent = nullptr;
    }
    
    
    assetManager->registerWithResidencySet(residency);
    
    
    // Setup camera
    _camera.position = {0.0f, 0.0f, 2.0f};
    _camera.target = {0.0f, 0.0f, 0.0f};
}

// Update (every frame)
void SpheresScene::update(float dt) {
    auto view = _registry.view<TransformComponent>();
    
    float time = _renderer->totalTime();
    float radius = 5.0f;
    float height = 1.0f;
    float speed = 0.3f;
    
    float theta = time * speed;
    
    _camera.position = simd_make_float3(
                                        radius * sin(theta),
                                        height + sin(theta) * 0.5f,
                                        radius * cos(theta)
                                        );
    
    _camera.target = simd_make_float3(0.0f, 0.0f, 0.0f);
    
    int index = 0;
    
    view.each([&](auto entity, TransformComponent &t) {
        float wave = sinf(_renderer->totalTime() * 2.0f + index * 0.8f);
        t.position.y = wave * 0.5f;
        t.rotation.y += dt;
        t.rotation.x += dt * 0.5f;
        
        index++;
    });
    
    auto viewMesh = _registry.view<TransformComponent, MeshComponent>();
    
    viewMesh.each([&](auto entity, TransformComponent &t, MeshComponent &m) {
        float wave = sinf(_renderer->totalTime() * 2.0f + index * 0.8f);
        
        if (wave > 0) {
            m.materialAddress = _fabricMatAddress;
        } else {
            m.materialAddress = _simpleMatAddress;
        }
        index++;
        
    });
    
    
}

// Render (every frame) --

void SpheresScene::render(Renderer *renderer) {
    if (!_monkeyMesh)
        return;
    
    auto encoder = renderer->currentEncoder();
    auto argTable = renderer->argumentTable();
    uint32_t frameIndex = renderer->context()->frameIndex();
    
    InstanceData *instanceDataPtr =
    (InstanceData *)_instanceBuffers[frameIndex]->contents();
    
    auto view = _registry.view<TransformComponent, MeshComponent>();
    
    uint32_t instanceCount = 0;
    
    view.each(
              [&](auto entity, const TransformComponent &t, const MeshComponent &m) {
                  matrix_float4x4 translation = matrix4x4_translation(t.position);
                  
                  matrix_float4x4 rotX = matrix4x4_rotation(t.rotation.x, 1, 0, 0);
                  matrix_float4x4 rotY = matrix4x4_rotation(t.rotation.y, 0, 1, 0);
                  matrix_float4x4 rotZ = matrix4x4_rotation(t.rotation.z, 0, 0, 1);
                  matrix_float4x4 rotation =
                  matrix_multiply(rotZ, matrix_multiply(rotY, rotX));
                  
                  matrix_float4x4 scale = matrix4x4_scale(t.scale);
                  matrix_float4x4 modelMatrix =
                  matrix_multiply(translation, matrix_multiply(rotation, scale));
                  instanceDataPtr[instanceCount].modelMatrix = modelMatrix;
                  
                  matrix_float3x3 upperLeft = matrix3x3_upper_left(modelMatrix);
                  
                  instanceDataPtr[instanceCount].materialAddress = m.materialAddress;
                  
                  matrix_float3x3 normalM = matrix_inverse_transpose(upperLeft);
                  instanceDataPtr[instanceCount].normalMatrix = matrix_make_rows(
                                                                                 normalM.columns[0].x, normalM.columns[1].x, normalM.columns[2].x, 0,
                                                                                 normalM.columns[0].y, normalM.columns[1].y, normalM.columns[2].y, 0,
                                                                                 normalM.columns[0].z, normalM.columns[1].z, normalM.columns[2].z, 0,
                                                                                 0, 0, 0, 1);
                  instanceCount++;
              });
    
    if (instanceCount == 0)
        return;
    
    // Bind instance buffers
    argTable->setAddress(_instanceBuffers[frameIndex]->gpuAddress(), BufferIndexInstanceData);
    
    argTable->setAddress(_monkeyMesh->vertexAddress(), BufferIndexVertices);
    
    for (const auto &submesh : _monkeyMesh->submeshes()) {
        encoder->drawIndexedPrimitives(
                                       MTL::PrimitiveTypeTriangle, submesh.indexCount, submesh.indexType,
                                       _monkeyMesh->indexAdress(), _monkeyMesh->indexBuffer()->length(),
                                       instanceCount);
    }
}
} // namespace Paloma
