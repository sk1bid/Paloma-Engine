//
//  MTKViewDelegate.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "MTKViewDelegate.h"
#include "AssetManager.hpp"
#include "Bridge.hpp"
#include "Context.hpp"
#include "PipelineCache.hpp"
#include "ShaderTypes.h"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include "AAPLMathUtilities.h"
#include <simd/simd.h>

@implementation PalomaViewDelegate {
    Paloma::Context* _context;
    Paloma::PipelineCache _pipelineCache;
    Paloma::AssetManager _assetManager;
    
    MTL::DepthStencilState* _depthState;
    MTL::Buffer* _uniformBuffers[Paloma::MaxFramesInFlight];
    bool _resourcesLoaded;
}
- (instancetype)initWithDevice:(id<MTLDevice>)device {
    self = [super init];
    if (self) {
        _context = new Paloma::Context();
        MTL::Device* cppDevice = (__bridge MTL::Device*)device;
        
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
                                                               sizeof(FrameUniforms),
                                                               MTL::ResourceStorageModeShared);
            _uniformBuffers[i]->setLabel(NS::String::string("UniformBuffer", NS::UTF8StringEncoding));
        }
        
        auto depthDesc = MTL::DepthStencilDescriptor::alloc()->init();
        depthDesc->setDepthCompareFunction(MTL::CompareFunctionLess);
        depthDesc->setDepthWriteEnabled(true);
        _depthState = _context->device()->newDepthStencilState(depthDesc);
        depthDesc->release();
        if (_depthState) {
            NSLog(@"✅ DepthStencilState created!");
        }
        
        // Load test primitive
        Paloma::Mesh* testMesh = _assetManager.getPrimitive("box");
        if (!testMesh){
            NSLog(@"Failed to load test mesh");
        }
        else{
            NSLog(@"Test mesh loaded");
        }
        
        MTL::ResidencySet* residency = _context->residencySet();
        for (uint32_t i = 0; i < Paloma::MaxFramesInFlight; i++) {
            residency->addAllocation(_uniformBuffers[i]);
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
    if (!_context) return;
    
    // -- Prepare Data for Shader --
    static float rotation = 0.0f;
    rotation += 0.01f;
    
    CGSize size = view.drawableSize;
    float aspect = size.width / size.height;
    
    // -- Prepare Matrix --
    
    // View Matrix
    simd_float3 eye = {0.0f, 0.0f, 3.0f};
    simd_float3 target = {0.0f, 0.0f, 0.0f};
    simd_float3 up = {0.0f, 1.0f, 0.0f};
    simd_float4x4 viewMatrix  = matrix_look_at_right_hand(eye, target, up);
    
    // Projection Matrix
    float fov = 65.0f * (M_PI / 180.0f); // into radians
    float near = 0.1f;
    float far = 100.0f; // distance of cutting
    simd_float4x4 projMatrix = matrix_perspective_right_hand(fov,
                                                             aspect,
                                                             near,
                                                             far);
    
    FrameUniforms uniforms;
    uniforms.viewMatrix = viewMatrix;
    uniforms.projectionMatrix = projMatrix;
    uniforms.cameraPosition = eye;
    uniforms.time = rotation;
    
    
    auto renderPassDesc = Bridge::currentRenderPassDescriptor((__bridge void*)view);
    auto drawable = Bridge::currentDrawable((__bridge void*)view);
    auto layerResidency = Bridge::layerResidencySet((__bridge void*)view);
    
    if (!renderPassDesc || !drawable) return;
    
    _context->beginFrame();
    
    auto encoder = _context->commandBuffer()->renderCommandEncoder(renderPassDesc);
    
    auto pipeline = _pipelineCache.getDefault();
    if (pipeline) {
        encoder->setRenderPipelineState(pipeline);
        
        encoder->setDepthStencilState(_depthState);
        encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        encoder->setCullMode(MTL::CullModeBack);
    }

    
    // -- bind resources for shader --
    
    /// get current frame index for uniform buffer
    uint32_t frameIndex = _context->frameIndex();
    MTL::Buffer* currentUniforms = _uniformBuffers[frameIndex];
    
    // copy setted uniforms to currentUniforms
    memcpy(currentUniforms->contents(), &uniforms, sizeof(FrameUniforms));
    
    // link argument table to encoder
    auto argTable = _context->argumentTable();
    encoder->setArgumentTable(argTable,
                              MTL::RenderStageVertex | MTL::RenderStageFragment);
    
    argTable->setAddress(currentUniforms->gpuAddress(), BufferIndexUniforms);
    
    // -- Draw mesh --
    if (_resourcesLoaded) {
        Paloma::Mesh* mesh = _assetManager.getPrimitive("box");
        
        if (mesh){
            // bind vertices
            argTable->setAddress(mesh->vertexAddress(), BufferIndexVertices);
            
            for (const auto& submesh : mesh->submeshes()) {
                encoder->drawIndexedPrimitives(
                                               MTL::PrimitiveTypeTriangle,
                                               submesh.indexCount,
                                               submesh.indexType,
                                               mesh->indexAdress(),
                                               submesh.indexCount * sizeof(uint16_t)
                                               );
            }
        }
    }
    
    encoder->endEncoding();
    
    _context->endFrame(drawable, layerResidency);
}
- (void)mtkView:(MTKView *)view drawableSizeWillChange:(CGSize)size {
    //
}
@end
