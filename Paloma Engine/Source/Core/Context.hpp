//
//  Context.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

// Provides access to Metal 4 Core Structure


#pragma once

#include "Types.hpp"

// Optimized building - using forward declarations
namespace MTL {
class Device;
class ResidencySet;
class SharedEvent;
}

namespace MTL4 {
class CommandQueue;
class CommandBuffer;
class CommandAllocator;
class Compiler;
class ArgumentTable;
}

namespace CA {
class MetalDrawable;
}

// Context Class

namespace Paloma {

class Context {
public:
    // -- Lifecycle --
    
    Context() = default;
    ~Context();
    
    /// Coping is forbiden
    Context(const Context&) = delete;
    Context& operator=(const Context&) = delete;
    
    // -- Init --
    
    /// init all Metal4 objects
    bool init();
    
    /// Release all resources
    void shutdown();
    
    // -- Frame Management --
    
    ///  Begin new frame
    void beginFrame();
    
    /// End frame and present on screen
    /// @param drawable - current drawable from MTKView
    /// @param layerResidency - ResidencySey from CAMetalLayer
    void endFrame(CA::MetalDrawable* drawable, MTL::ResidencySet* layerResidency);
    
    // -- Accessors --
    
    MTL::Device* device() const {return _device;}
    MTL4::CommandQueue* commandQueue() const {return _commandQueue;}
    MTL4::CommandBuffer* commandBuffer() const {return _commandBuffer;}
    MTL4::Compiler* compiler() const {return _compiler;}
    MTL4::ArgumentTable* argumentTable() const {return _argumentTable;}
    MTL::ResidencySet* residencySet() const {return _residencySet;}
    
    /// Index of circle buffer. Use to choose current CommandAllocator
    /// _frameIndex = 0,1,2,0,1,2,0...
    uint32_t frameIndex() const {return _frameIndex;}
    
    /// Infinite counter of frames. Use to sync CPU and GPU work
    /// _frameNumber = 3,4,5,6...
    /// Begin with 3 to make shure that 0-frame is fully drawed and allocator is ready
    uint64_t frameNumber() const {return _frameNumber;}
    
private:
    
    // -- Metal 4 Objects --
    
    MTL::Device* _device = nullptr;
    MTL4::CommandQueue* _commandQueue = nullptr;
    MTL4::CommandBuffer* _commandBuffer = nullptr;
    MTL4::Compiler* _compiler = nullptr;
    MTL4::ArgumentTable* _argumentTable = nullptr;
    MTL::ResidencySet* _residencySet = nullptr;
    MTL::SharedEvent* _frameEvent = nullptr;
    
    ///Triple-buffered allocators
    MTL4::CommandAllocator* _allocators[MaxFramesInFlight] = {};
    
    // -- Frame State --
    
    uint32_t _frameIndex = 0; /// 0,1,2 - cycle
    uint64_t _frameNumber = MaxFramesInFlight; /// begin with N
    
    // -- Private Helpers --
    
    void waitForFrame(uint64_t frameNumber);
};
}
