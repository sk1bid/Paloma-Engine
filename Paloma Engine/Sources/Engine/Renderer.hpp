//
//  Renderer.hpp
//  Paloma Engine
//
//  Created by Artem on 20.01.2026.
//

#pragma once
#include "BufferUtilites.hpp"
#include "Camera.hpp"
#include "Material.hpp"
#include "Mesh.hpp"
#include "Metal/Metal.hpp"
#include "MetalKit/MetalKit.hpp"
#include "AAPLMathUtilities.h"
#include "FlyCamera.hpp"
#include "Scene.hpp"

class RendererInterface : public MTK::ViewDelegate {
public:
    virtual ~RendererInterface() = default;
    virtual void configure(MTK::View *pView) = 0;
};

RendererInterface *CreateRenderer(MTL::Device *pDevice);

class Metal4Renderer : public RendererInterface {
public:
    Metal4Renderer(MTL::Device *pDevice);
    virtual ~Metal4Renderer() override = default;
    
    virtual void drawInMTKView(MTK::View *pView) override;
    virtual void drawableSizeWillChange(MTK::View *pView, CGSize size) override;
    
    virtual void configure(MTK::View *pView) override;
    
private:
    void makeResources();
    void makeSceneResourcesResident(Scene* scene);
    NS::SharedPtr<MTL::RenderPipelineState>
    makePipelineState(Mesh *pMesh, Material *pMaterial);
    void updateCamera(float deltaTime);
    
private:
    NS::SharedPtr<MTL::Device> _pDevice;
    NS::SharedPtr<MTL4::CommandQueue> _pCommandQueue;
    NS::SharedPtr<MTL4::CommandBuffer> _pCommandBuffer;
    std::vector<NS::SharedPtr<MTL4::CommandAllocator>> _pCommandAllocators;
    
    NS::SharedPtr<MTL::Library> _pLibrary;
    NS::SharedPtr<MTL4::Compiler> _pCompiler;
    
    NS::SharedPtr<MTL::ResidencySet> _pResidencySet;
    NS::SharedPtr<MTL::SharedEvent> _pFrameCompletionEvent;
    
    NS::SharedPtr<MTL4::ArgumentTable> _pVertexArgumentTable;
    NS::SharedPtr<MTL4::ArgumentTable> _pFragmentArgumentTable;

    MTL::PixelFormat _colorPixelFormat;
    MTL::PixelFormat _depthStencilPixelFormat;
    uint32_t _rasterSampleCount;
    
    NS::SharedPtr<MTL::DepthStencilState> _pDepthStencilStates[3];
    
    static constexpr uint64_t kMaxFramesInFlight = 3;
    std::shared_ptr<Scene> _pScene;
    PerspectiveCamera _camera;
    FlyCamera _flyCamera;
    RingBuffer *_pConstantBuffers[kMaxFramesInFlight];
    RingBuffer *_pMaterialsBuffer;
    uint64_t _frameIndex = 0;
    
    bool _hasPreparedResources = false;
    bool _iblReady = false;
    double _lastRenderTime = 0.0;
    double _time = 0.0;
};
