//
//  Context.cpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#include "Context.hpp"

#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

namespace Paloma {

// -- Destructor --
Context::~Context() { shutdown(); }

// -- Init Metal 4 --

bool Context::init(MTL::Device *device) {
    
    
    // -- Get GPU device from MTKView--
    _device = device;
    if (!_device) {
        return false;
    }
    
    // -- Check Metal 4 Support --
    if (!_device->supportsFamily(MTL::GPUFamilyMetal4)) {
        _device = nullptr;
        return false; // Metal 4 not supported on this device
    }
    
    // -- Create CommandQueue --
    _commandQueue = _device->newMTL4CommandQueue();
    if (!_commandQueue) {
        shutdown();
        return false;
    }
    
    // -- Create N CommandAllocator (Triple Buffering)
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        _allocators[i] = _device->newCommandAllocator();
        if (!_allocators[i]) {
            shutdown();
            return false;
        }
    }
    
    // -- Create Command Buffer (reused)
    _commandBuffer = _device->newCommandBuffer();
    if (!_commandBuffer) {
        shutdown();
        return false;
    }
    
    // -- Create Compiler for shaders
    auto compilerDesc = MTL4::CompilerDescriptor::alloc()->init();
    _compiler = _device->newCompiler(compilerDesc, nullptr);
    compilerDesc->release();
    /// descriptors - set of rules how to create object in Metal.
    /// All descriptors creating in ::alloc()->init() pattern
    /// After newObect operation you must release used descriptor
    
    if (!_compiler) {
        shutdown();
        return false;
    }
    
    // Create Argument Table for resource binding
    
    auto argTableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
    argTableDesc->setMaxBufferBindCount(
                                        16); /// Example buffers: Unitforms, Instances, Vertices, Materials
    argTableDesc->setMaxTextureBindCount(
                                         16); /// Albedo, Normal, Roughness, Environment
    argTableDesc->setMaxSamplerStateBindCount(8); /// Linear, Nearest
    
    _argumentTable = _device->newArgumentTable(argTableDesc, nullptr);
    argTableDesc->release();
    
    if (!_argumentTable) {
        shutdown();
        return false;
    }
    
    // -- Create Residency Set for GPU resources
    auto residencyDesc = MTL::ResidencySetDescriptor::alloc()->init();
    residencyDesc->setInitialCapacity(64);
    
    _residencySet = _device->newResidencySet(residencyDesc, nullptr);
    residencyDesc->release();
    
    if (!_residencySet) {
        shutdown();
        return false;
    }
    
    // Add our residency set to the queue
    _commandQueue->addResidencySet(_residencySet);
    
    // -- Create Shared Event for CPU<>GPU sync
    _frameEvent = _device->newSharedEvent();
    if (!_frameEvent) {
        shutdown();
        return false;
    }
    
    // Set initial signaled value
    // This allows first frames to not wait unnecessarily
    _frameEvent->setSignaledValue(MaxFramesInFlight - 1);
    
    return true;
}

// -- Shutdown - release all Metal resources

void Context::shutdown() {
    // Release in reverse
    if (_frameEvent) {
        _frameEvent->release();
        _frameEvent = nullptr;
    }
    
    if (_residencySet) {
        _residencySet->release();
        _residencySet = nullptr;
    }
    
    if (_argumentTable) {
        _argumentTable->release();
        _argumentTable = nullptr;
    }
    
    if (_compiler) {
        _compiler->release();
        _compiler = nullptr;
    }
    
    if (_commandBuffer) {
        _commandBuffer->release();
        _commandBuffer = nullptr;
    }
    
    for (uint32_t i = 0; i < MaxFramesInFlight; i++) {
        if (_allocators[i]) {
            _allocators[i]->release();
            _allocators[i] = nullptr;
        }
    }
    
    if (_commandQueue) {
        _commandQueue->release();
        _commandQueue = nullptr;
    }
    
    if (_device) {
        _device = nullptr;
    }
}

// -- WaitForFrame - waitng ending of frame on GPU --

void Context::waitForFrame(uint64_t frameNumber) {
    
    /// if GPU is not reach for this frame - wait
    if (_frameEvent->signaledValue() < frameNumber) {
        _frameEvent->waitUntilSignaledValue(frameNumber, 1000); // wait max 1 sec
    }
}

// -- Begin Frame --

void Context::beginFrame() {
    /// wait to allocator is free
    uint64_t waitFrame = (_frameNumber >= MaxFramesInFlight)
    ? (_frameNumber - MaxFramesInFlight)
    : 0;
    
    waitForFrame(waitFrame); /// wait N-3 frame
    
    /// Reset allocator of current frame
    _allocators[_frameIndex]->reset();
    
    /// Begin command buffer
    _commandBuffer->beginCommandBuffer(_allocators[_frameIndex]);
}

// --End Frame - ending frame and push to GPU --

void Context::endFrame(CA::MetalDrawable *drawable,
                       MTL::ResidencySet *layerResidency) {
    
    /// Use layer's residency set per-frame (our _residencySet is added to queue
    /// in init)
    if (layerResidency) {
        _commandBuffer->useResidencySet(layerResidency);
    }
    
    /// Close command buffer (AFTER residency sets)
    _commandBuffer->endCommandBuffer();
    
    /// Wait for drawable (VSync)
    _commandQueue->wait(drawable);
    
    /// Transfer buffer on GPU
    const MTL4::CommandBuffer *buffers[] = {_commandBuffer};
    _commandQueue->commit(buffers, 1);
    
    /// Signal frame ending
    _commandQueue->signalEvent(_frameEvent, _frameNumber);
    _commandQueue->signalDrawable(drawable);
    
    /// Show frame
    drawable->present();
    
    /// Next frame
    _frameNumber++;
    _frameIndex = (_frameIndex + 1) % MaxFramesInFlight;
}

} // namespace Paloma
