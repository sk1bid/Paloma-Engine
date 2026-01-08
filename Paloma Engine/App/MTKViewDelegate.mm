//
//  MTKViewDelegate.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "MTKViewDelegate.h"
#include "AAPLMathUtilities.h"
#include "AssetManager.hpp"
#include "Context.hpp"
#include "PipelineCache.hpp"
#include "ShaderTypes.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <simd/simd.h>

@implementation PalomaViewDelegate {
    // C++ objects
    Paloma::Context *_context;
    Paloma::PipelineCache _pipelineCache;
    Paloma::AssetManager _assetManager;
    
    MTL::DepthStencilState *_depthState;
    MTL::Buffer *_uniformBuffers[Paloma::MaxFramesInFlight];
    MTL::Buffer *_instanceBuffers[Paloma::MaxFramesInFlight];
    bool _resourcesLoaded;
    
    // ObjC frame objects - stored as ivars to prevent ARC optimization issues
    MTL4RenderPassDescriptor *_currentRenderPassDesc;
    id<CAMetalDrawable> _currentDrawable;
    id<MTLResidencySet> _currentLayerResidency;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    self = [super init];
    if (self) {
        _context = new Paloma::Context();
        MTL::Device *cppDevice = (__bridge MTL::Device *)device;
        
        if (!_context->init(cppDevice)) {
            NSLog(@"Failed to initialize Metal 4 Context!");
            delete _context;
            _context = nullptr;
            return nil;
        }
        
        NSLog(@"Metal 4 Context initialized!");
        
        // init pipeline cache
        _pipelineCache.init(_context->device(), _context->compiler());
        
        NSLog(@"Pipeline cache initialized!");
        
        _assetManager.init(_context->device());
        
        // -- Create buffers for uniforms --
        for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
            _uniformBuffers[i] = _context->device()->newBuffer(
                                                               sizeof(FrameUniforms), MTL::ResourceStorageModeShared);
            _uniformBuffers[i]->setLabel(
                                         NS::String::string("UniformBuffer", NS::UTF8StringEncoding));
        }
        
        for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++){
            _instanceBuffers[i] = _context->device()->newBuffer(sizeof(InstanceData),
                                                                MTL::ResourceStorageModeShared);
            _instanceBuffers[i]->setLabel(
                 NS::String::string("InstanceDataBuffer", NS::UTF8StringEncoding));
        }
        
        auto depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
        depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
        depthDesc->setDepthWriteEnabled(true);
        _depthState = _context->device()->newDepthStencilState(depthDesc);
        depthDesc->release();
        if (_depthState) {
            NSLog(@"DepthStencilState created!");
        }
        
        // Load test primitive
        Paloma::Mesh *testMesh = _assetManager.getPrimitive("box");
        if (!testMesh) {
            NSLog(@"Failed to load test mesh");
        } else {
            NSLog(@"Test mesh loaded");
        }
        
        MTL::ResidencySet *residency = _context->residencySet();
        for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
            residency->addAllocation(_uniformBuffers[i]);
        }
        
        for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
          residency->addAllocation(_instanceBuffers[i]);
        }
        
        _assetManager.registerWithResidencySet(residency);
        
        _resourcesLoaded = (testMesh != nullptr);
        NSLog(@"Resources registered with ResidencySet");
    }
    return self;
}

- (void)dealloc {
    if (_depthState) {
        _depthState->release();
        _depthState = nullptr;
    }
    // Release instance buffers
    for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
      if (_instanceBuffers[i]) {
        _instanceBuffers[i]->release();
        _instanceBuffers[i] = nullptr;
      }
    }
    
    for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
        if (_uniformBuffers[i]) {
            _uniformBuffers[i]->release();
            _uniformBuffers[i] = nullptr;
        }
    }
    
    _assetManager.shutdown();
    _pipelineCache.shutdown();
    
    if (_context) {
        _context->shutdown();
        delete _context;
        _context = nullptr;
    }
}

- (void)drawInMTKView:(MTKView *)view {
    if (!_context)
        return;
    
    // -- Get frame resources (stored as ivars for ARC lifetime) --
    _currentRenderPassDesc = view.currentMTL4RenderPassDescriptor;
    _currentDrawable = view.currentDrawable;
    _currentLayerResidency = ((CAMetalLayer *)view.layer).residencySet;
    
    if (!_currentRenderPassDesc || !_currentDrawable)
        return;
    
    // Convert to C++ types
    auto renderPassDesc =
    (__bridge MTL4::RenderPassDescriptor *)_currentRenderPassDesc;
    auto drawable = (__bridge CA::MetalDrawable *)_currentDrawable;
    auto layerResidency = (__bridge MTL::ResidencySet *)_currentLayerResidency;
    
    // -- Prepare Data for Shader --
    static float rotation = 0.0f;
    rotation += 0.01f;
    
    CGSize size = view.drawableSize;
    float aspect = size.width / size.height;
    
    // View Matrix
    simd_float3 eye = {0.0f, 0.0f, 3.0f};
    simd_float3 target = {0.0f, 0.0f, 0.0f};
    simd_float3 up = {0.0f, 1.0f, 0.0f};
    simd_float4x4 viewMatrix = matrix_look_at_right_hand(eye, target, up);
    
    // Projection Matrix
    float fov = 65.0f * (M_PI / 180.0f);
    float near = 0.1f;
    float far = 100.0f;
    simd_float4x4 projMatrix =
    matrix_perspective_right_hand(fov, aspect, near, far);
    
    FrameUniforms uniforms = {};
    uniforms.viewMatrix = viewMatrix;
    uniforms.projectionMatrix = projMatrix;
    uniforms.cameraPosition = eye;
    uniforms.time = rotation;
    
    // -- Prepare instance data --
    
    simd_float4x4 rotationY = matrix4x4_rotation(rotation, 0.0f, 1.0f, 0.0f);
    simd_float4x4 rotationX = matrix4x4_rotation(rotation * 0.5f, 1.0f, 0.0f, 0.0f);
    
    simd_float4x4 modelMatrix = simd_mul(rotationX, rotationY);
    simd_float4x4 normalMatrix = modelMatrix;
    
    // Fill instance data struct
    InstanceData instanceData = {};
    instanceData.modelMatrix = modelMatrix;
    instanceData.normalMatrix = normalMatrix;
    
    // -- Begin Frame --
    _context->beginFrame();
    
    auto encoder =
    _context->commandBuffer()->renderCommandEncoder(renderPassDesc);
    if (!encoder) {
        return;
    }
    
    auto pipeline = _pipelineCache.getDefault();
    if (pipeline) {
        encoder->setRenderPipelineState(pipeline);
        encoder->setDepthStencilState(_depthState);
        encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        encoder->setCullMode(MTL::CullModeBack);
    }
    
    // -- Bind resources --
    uint32_t frameIndex = _context->frameIndex();
    MTL::Buffer *currentUniforms = _uniformBuffers[frameIndex];
    memcpy(currentUniforms->contents(), &uniforms, sizeof(FrameUniforms));
    
    // Copy instance data to current frame's buffer
    MTL::Buffer *currentInstance = _instanceBuffers[frameIndex];
    memcpy(currentInstance->contents(), &instanceData, sizeof(InstanceData));
    
    auto argTable = _context->argumentTable();
    encoder->setArgumentTable(argTable,
                              MTL::RenderStageVertex | MTL::RenderStageFragment);
    argTable->setAddress(currentUniforms->gpuAddress(), BufferIndexUniforms);
    
    argTable->setAddress(currentInstance->gpuAddress(), BufferIndexInstanceData);
    
    // -- Draw mesh --
    if (_resourcesLoaded) {
        Paloma::Mesh *mesh = _assetManager.getPrimitive("box");
        if (mesh) {
            argTable->setAddress(mesh->vertexAddress(), BufferIndexVertices);
            for (const auto &submesh : mesh->submeshes()) {
                encoder->drawIndexedPrimitives(
                                               MTL::PrimitiveTypeTriangle, submesh.indexCount, submesh.indexType,
                                               mesh->indexAdress(), submesh.indexCount * sizeof(uint16_t));
            }
        }
    }
    
    encoder->endEncoding();
    _context->endFrame(drawable, layerResidency);
}

- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

@end
