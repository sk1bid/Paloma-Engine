//
//  Bridge.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//
//  Bridge between Objective-C and C++ Metal APIs.
//  NOTE: Returned pointers are only valid for the current autorelease pool
//  scope. The caller must ensure the ObjC source object stays alive during use.
//

#import "Bridge.hpp"
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>
#import <QuartzCore/QuartzCore.h>
#include "Bridge.hpp"
#import <Foundation/Foundation.h>

namespace Bridge {

// -- Current RenderPassDescriptor --
// Returns a C++ pointer. Caller must keep MTKView alive during use.

MTL4::RenderPassDescriptor *currentRenderPassDescriptor(void *mtkView) {
  MTKView *view = (__bridge MTKView *)mtkView;
  MTL4RenderPassDescriptor *desc = view.currentMTL4RenderPassDescriptor;
  return desc ? (__bridge MTL4::RenderPassDescriptor *)desc : nullptr;
}

// -- Current Drawable --

CA::MetalDrawable *currentDrawable(void *mtkView) {
  MTKView *view = (__bridge MTKView *)mtkView;
  id<CAMetalDrawable> drawable = view.currentDrawable;
  return drawable ? (__bridge CA::MetalDrawable *)drawable : nullptr;
}

// -- Layer Residency Set --

MTL::ResidencySet *layerResidencySet(void *mtkView) {
  MTKView *view = (__bridge MTKView *)mtkView;
  CAMetalLayer *layer = (CAMetalLayer *)view.layer;
  id<MTLResidencySet> residency = layer.residencySet;
  return residency ? (__bridge MTL::ResidencySet *)residency : nullptr;
}

// -- Wait for Event --

void waitForEvent(MTL::SharedEvent *event, uint64_t value, uint64_t timeoutMS) {
  id<MTLSharedEvent> objcEvent = (__bridge id<MTLSharedEvent>)event;
  [objcEvent waitUntilSignaledValue:value timeoutMS:timeoutMS];
}

// -- Get View Size --

void getViewSize(void *mtkView, uint32_t *outWidth, uint32_t *outHeight) {
  MTKView *view = (__bridge MTKView *)mtkView;
  CGSize size = view.drawableSize;
  *outWidth = (uint32_t)size.width;
  *outHeight = (uint32_t)size.height;
}

std::string getBundleResourcePath(const char* name, const char* ext, const char* directory) {
    @autoreleasepool {
        NSString* nsName = [NSString stringWithUTF8String:name];
        NSString* nsExt = [NSString stringWithUTF8String:ext];
        NSString* nsDir = directory ? [NSString stringWithUTF8String:directory] : nil;
        
        NSString* path = [[NSBundle mainBundle] pathForResource:nsName
                                                         ofType:nsExt
                                                    inDirectory:nsDir];
        if (path) {
            return std::string([path UTF8String]);
        }
        return "";
    }
}

} // namespace Bridge
