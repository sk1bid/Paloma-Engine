//
//  Bridge.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import <MetalKit/MetalKit.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>
#import "Bridge.hpp"

namespace Bridge {

// -- Current RenderPassDescriptor --

MTL4::RenderPassDescriptor* currentRenderPassDescriptor(void* mtkView) {
    
    // Convert void* to MTKView*
    MTKView* view = (__bridge MTKView*)mtkView;
    
    // Get OBJ-C RenderPassDescriptor
    MTL4RenderPassDescriptor* objcDesc = view.currentMTL4RenderPassDescriptor;
    
    // Convert into C++ type
    return (__bridge MTL4::RenderPassDescriptor*)objcDesc;
}

// -- Current Drawable --

CA::MetalDrawable* currentDrawable(void* mtkView) {
    MTKView* view = (__bridge MTKView*)mtkView;
    
    /// MTKView prepare decriptor, optimized for your curernt window
    id<CAMetalDrawable> objcDrawable = view.currentDrawable;
    
    return (__bridge CA::MetalDrawable*)objcDrawable;
}

// -- Layer Residency Set --

MTL::ResidencySet* layerResidencySet(void* mtkView){
    MTKView* view = (__bridge MTKView*)mtkView;
    
    /// Get CAMetalLayer
    CAMetalLayer* layer = (CAMetalLayer*)view.layer;
    
    /// You should not register texture of the window every frame
    /// Link once residency set to your CommandQueue
    /// Metal garants you that memory for window should be ready for drawing
    id<MTLResidencySet> objcResidency = layer.residencySet;
    
    return (__bridge MTL::ResidencySet*)objcResidency;
}
// -- Wait for Event

void waitForEvent(MTL::SharedEvent* event, uint64_t value, uint64_t timeoutMS){
    /// Convert C++ into Obj-C
    id<MTLSharedEvent> objcEvent = (__bridge id<MTLSharedEvent>)event;
    
    /// Call OBJ-C method
    [objcEvent waitUntilSignaledValue:value timeoutMS:timeoutMS];
}

// -- Get View Size --

void getViewSize(void* mtkView, uint32_t* outWidth, uint32_t* outHeight) {
    MTKView* view = (__bridge MTKView*)mtkView;
    
    /// Drawable size knows about Retina scale
    /// Drawable size is a real resolution in pixels
    CGSize size = view.drawableSize;
    
    *outWidth = (uint32_t)size.width;
    *outHeight = (uint32_t)size.height;
    
}
}
