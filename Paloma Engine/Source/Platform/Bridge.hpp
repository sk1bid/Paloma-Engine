//
//  Bridge.hpp
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#pragma once

#include <cstdint>

namespace MTL {
class ResidencySet;
class SharedEvent;
}

namespace MTL4 {
class RenderPassDescriptor;
}

namespace CA {
class MetalDrawable;
}

// -- Bridge Functions - call in C++, realized in Obj-C++

namespace Bridge {

/// Get current RenderPassDescriptor from MTKView
/// @Param mtkView - pointer on  MTKView
/// @return MTL4::RenderPassDescriptor*
MTL4::RenderPassDescriptor* currentRenderPassDescriptor(void* mtkView);

/// Get current Drawable from MTKView
/// @Param mtkView - pointer on MTKView
/// @return CA::MetalDrawable*
CA::MetalDrawable* currentDrawable(void* mtkView);

/// Get ResidencySet from CAMetalLayer
/// @param mtkView - pointer on MTKView
/// @return MTL::ResidencySet*
MTL::ResidencySet* layerResidencySet(void* mtkView);

/// Wait for SharedEvent
/// @param event - event for waiting
/// @param value - value for waiting
/// @param timeoutMS - timeout in ms
void waitForEvent(MTL::SharedEvent* event, uint64_t value, uint64_t timeoutMS);

/// Get size of view
/// @param mtkView - pointer on MTKView
/// @param outWidth - output width
/// @param outHeight - output height
void getViewSize(void* mtkView, uint32_t* outWidth, uint32_t* outHeight);
}
