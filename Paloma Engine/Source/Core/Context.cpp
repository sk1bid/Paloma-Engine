//
//  Context.cpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#include "Context.hpp"

#include <Metal/Metal.hpp>

namespace Paloma {

// -- Destructor --
Context::~Context() {
    shutdown();
}

// -- Init Metal 4 --

bool Context::init() {
    
    // -- Get GPU --
    _device = MTL::CreateSystemDefaultDevice();
    if (!_device){
        return false;
    }
    
    // -- Check Metal 4 Support --
    if (!_device->supportsFamily(MTL::GPUFamilyMetal4)){
        _device->release();
        _device = nullptr;
        return false; // Metal 4 not supported on this device
    }
    
    // -- Create CommandQueue --
    _commandQueue = _device->newMTL4CommandQueue();
    if (!_commandQueue){
        shutdown();
        return false;
    }
    
    // -- Create N CommandAllocator (Triple Buffering)
    for (uint32_t i = 0; i < MaxFramesInFlight; i++){
        _allocators[i] = _device->newCommandAllocator();
        if (!_allocators[i]){
            shutdown();
            return false;
        }
    }
    
    // -- Create Command Buffer (reused)
    _commandBuffer = _device->newCommandBuffer();
    if (!_commandBuffer){
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
    
    if (!_compiler){
        shutdown();
        return false;
    }
    
    // Create Argument Table for resource binding
    
    auto argTableDesc = MTL4::ArgumentTableDescriptor::alloc()->init();
    argTableDesc->setMaxBufferBindCount(16); /// Example buffers: Unitforms, Instances, Vertices, Materials
    argTableDesc->setMaxTextureBindCount(16); /// Albedo, Normal, Roughness, Environment
    argTableDesc->setMaxSamplerStateBindCount(8); /// Linear, Nearest
    
    _argumentTable = _device->newArgumentTable(argTableDesc, nullptr);
    argTableDesc->release();
    
    if (!_argumentTable){
        shutdown();
        return false;
    }
    
    // -- Create Residency Set for GPU resources
    auto residencyDesc = MTL::ResidencySetDescriptor::alloc()->init();
    residencyDesc->setInitialCapacity(64);
    
    _residencySet = _device->newResidencySet(residencyDesc, nullptr);
    residencyDesc->release();
    
    if (!_residencySet){
        shutdown();
        return false;
    }
    
    // -- Create Shared Event for CPU<>GPU sync
    _frameEvent = _device->newSharedEvent();
    if (!_frameEvent){
        shutdown();
        return false;
    }
    
    return true;
}

// -- Shutdown - release all Metal resources

void Context::shutdown(){
    // Release in reverse
    if (_frameEvent){
        _frameEvent->release();
        _frameEvent = nullptr;
    }
    
    if (_residencySet){
        _residencySet->release();
        _residencySet = nullptr;
    }
    
    if (_argumentTable){
        _argumentTable->release();
        _argumentTable = nullptr;
    }
    
    if (_compiler){
        _compiler->release();
        _compiler = nullptr;
    }
    
    if (_commandBuffer){
        _commandBuffer->release();
        _commandBuffer = nullptr;
    }
    
    for (int i = 0; i < MaxFramesInFlight; i++){
        _allocators[i]->release();
        _allocators[i] = nullptr;
    }
    
    if (_commandQueue){
        _commandQueue->release();
        _commandQueue = nullptr;
    }
}


}


