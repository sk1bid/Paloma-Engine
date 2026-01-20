/*
 * MetalKitPrivate.hpp
 */

#pragma once

#include <Metal/Metal.hpp>
#include <objc/runtime.h>

#define _MTK_PRIVATE_CLS(symbol) (Private::Class::s_k##symbol)
#define _MTK_PRIVATE_SEL(accessor) (Private::Selector::s_k##accessor)

#if defined(MTK_PRIVATE_IMPLEMENTATION)
#define _MTK_PRIVATE_VISIBILITY __attribute__((visibility("default")))
#define _MTK_PRIVATE_IMPORT __attribute__((weak_import))

#if __OBJC__
#define _MTK_PRIVATE_OBJC_LOOKUP_CLASS(symbol) \
    ((__bridge void*)objc_lookUpClass(#symbol))
#else
#define _MTK_PRIVATE_OBJC_LOOKUP_CLASS(symbol) objc_lookUpClass(#symbol)
#endif

#define _MTK_PRIVATE_DEF_CLS(symbol) \
    void* s_k##symbol _MTK_PRIVATE_VISIBILITY = _MTK_PRIVATE_OBJC_LOOKUP_CLASS(symbol);
#define _MTK_PRIVATE_DEF_SEL(accessor, symbol) \
    SEL s_k##accessor _MTK_PRIVATE_VISIBILITY = sel_registerName(symbol);
#define _MTK_PRIVATE_DEF_CONST(type, symbol)               \
    _NS_EXTERN type const MTK##symbol _MTK_PRIVATE_IMPORT; \
    type const                        MTK::symbol = (nullptr != &MTK##symbol) ? MTK##symbol : nullptr;
#else
#define _MTK_PRIVATE_DEF_CLS(symbol) extern void* s_k##symbol;
#define _MTK_PRIVATE_DEF_SEL(accessor, symbol) extern SEL s_k##accessor;
#define _MTK_PRIVATE_DEF_CONST(type, symbol)
#endif

namespace MTK
{
namespace Private
{
    namespace Class
    {
        _MTK_PRIVATE_DEF_CLS(MTKView);
        _MTK_PRIVATE_DEF_CLS(MTKTextureLoader);
        _MTK_PRIVATE_DEF_CLS(MTKMesh);
        _MTK_PRIVATE_DEF_CLS(MTKSubmesh);
        _MTK_PRIVATE_DEF_CLS(MTKMeshBuffer);
        _MTK_PRIVATE_DEF_CLS(MTKMeshBufferAllocator);
    } // namespace Class

    namespace Selector
    {
        _MTK_PRIVATE_DEF_SEL(device, "device");
        _MTK_PRIVATE_DEF_SEL(initWithDevice_, "initWithDevice:");
        _MTK_PRIVATE_DEF_SEL(newTextureWithContentsOfURL_options_error_,
            "newTextureWithContentsOfURL:options:error:");
        _MTK_PRIVATE_DEF_SEL(newTextureWithData_options_error_,
            "newTextureWithData:options:error:");
        _MTK_PRIVATE_DEF_SEL(newTextureWithMDLTexture_options_error_,
            "newTextureWithMDLTexture:options:error:");
        _MTK_PRIVATE_DEF_SEL(initWithMesh_device_error_, "initWithMesh:device:error:");
        _MTK_PRIVATE_DEF_SEL(newTextureWithCGImage_options_error_, "newTextureWithCGImage:options:error:");
        _MTK_PRIVATE_DEF_SEL(newTextureWithName_scaleFactor_bundle_options_error_, "newTextureWithName:scaleFactor:bundle:options:error:");
        _MTK_PRIVATE_DEF_SEL(newTexturesWithContentsOfURLs_options_error_, "newTexturesWithContentsOfURLs:options:error:");
        _MTK_PRIVATE_DEF_SEL(newMeshesFromAsset_device_sourceMeshes_error_,
            "newMeshesFromAsset:device:sourceMeshes:error:");
        _MTK_PRIVATE_DEF_SEL(vertexBuffers, "vertexBuffers");
        _MTK_PRIVATE_DEF_SEL(vertexDescriptor, "vertexDescriptor");
        _MTK_PRIVATE_DEF_SEL(submeshes, "submeshes");
        _MTK_PRIVATE_DEF_SEL(vertexCount, "vertexCount");
        _MTK_PRIVATE_DEF_SEL(name, "name");
        _MTK_PRIVATE_DEF_SEL(setName_, "setName:");
        _MTK_PRIVATE_DEF_SEL(primitiveType, "primitiveType");
        _MTK_PRIVATE_DEF_SEL(indexType, "indexType");
        _MTK_PRIVATE_DEF_SEL(indexBuffer, "indexBuffer");
        _MTK_PRIVATE_DEF_SEL(indexCount, "indexCount");
        _MTK_PRIVATE_DEF_SEL(mesh, "mesh");
        _MTK_PRIVATE_DEF_SEL(length, "length");
        _MTK_PRIVATE_DEF_SEL(allocator, "allocator");
        _MTK_PRIVATE_DEF_SEL(zone, "zone");
        _MTK_PRIVATE_DEF_SEL(buffer, "buffer");
        _MTK_PRIVATE_DEF_SEL(offset, "offset");
        _MTK_PRIVATE_DEF_SEL(setPaused_, "setPaused:");
        _MTK_PRIVATE_DEF_SEL(isPaused, "isPaused");
        _MTK_PRIVATE_DEF_SEL(clearColor, "clearColor");
        _MTK_PRIVATE_DEF_SEL(setClearColor_, "setClearColor:");
        _MTK_PRIVATE_DEF_SEL(clearDepth, "clearDepth");
        _MTK_PRIVATE_DEF_SEL(setClearDepth_, "setClearDepth:");
        _MTK_PRIVATE_DEF_SEL(clearStencil, "clearStencil");
        _MTK_PRIVATE_DEF_SEL(setClearStencil_, "setClearStencil:");
        _MTK_PRIVATE_DEF_SEL(colorPixelFormat, "colorPixelFormat");
        _MTK_PRIVATE_DEF_SEL(setColorPixelFormat_, "setColorPixelFormat:");
        _MTK_PRIVATE_DEF_SEL(depthStencilPixelFormat, "depthStencilPixelFormat");
        _MTK_PRIVATE_DEF_SEL(setDepthStencilPixelFormat_,
            "setDepthStencilPixelFormat:");
        _MTK_PRIVATE_DEF_SEL(depthStencilStorageMode, "depthStencilStorageMode");
        _MTK_PRIVATE_DEF_SEL(setDepthStencilStorageMode_, "setDepthStencilStorageMode:");
        _MTK_PRIVATE_DEF_SEL(sampleCount, "sampleCount");
        _MTK_PRIVATE_DEF_SEL(setSampleCount_, "setSampleCount:");
        _MTK_PRIVATE_DEF_SEL(currentDrawable, "currentDrawable");
        _MTK_PRIVATE_DEF_SEL(currentRenderPassDescriptor,
            "currentRenderPassDescriptor");
        _MTK_PRIVATE_DEF_SEL(currentMTL4RenderPassDescriptor,
            "currentMTL4RenderPassDescriptor");
        _MTK_PRIVATE_DEF_SEL(drawableSize, "drawableSize");
        _MTK_PRIVATE_DEF_SEL(setDrawableSize_, "setDrawableSize:");
        _MTK_PRIVATE_DEF_SEL(draw, "draw");
        _MTK_PRIVATE_DEF_SEL(delegate, "delegate");
        _MTK_PRIVATE_DEF_SEL(setDelegate_, "setDelegate:");
        _MTK_PRIVATE_DEF_SEL(drawInMTKView_, "drawInMTKView:");
        _MTK_PRIVATE_DEF_SEL(mtkView_drawableSizeWillChange_, "mtkView:drawableSizeWillChange:");
        _MTK_PRIVATE_DEF_SEL(framebufferOnly, "framebufferOnly");
        _MTK_PRIVATE_DEF_SEL(setFramebufferOnly_, "setFramebufferOnly:");
        _MTK_PRIVATE_DEF_SEL(depthStencilAttachmentTextureUsage, "depthStencilAttachmentTextureUsage");
        _MTK_PRIVATE_DEF_SEL(setDepthStencilAttachmentTextureUsage_, "setDepthStencilAttachmentTextureUsage:");
        _MTK_PRIVATE_DEF_SEL(multisampleColorAttachmentTextureUsage, "multisampleColorAttachmentTextureUsage");
        _MTK_PRIVATE_DEF_SEL(setMultisampleColorAttachmentTextureUsage_, "setMultisampleColorAttachmentTextureUsage:");
        _MTK_PRIVATE_DEF_SEL(releaseDrawables, "releaseDrawables");
        _MTK_PRIVATE_DEF_SEL(autoResizeDrawable, "autoResizeDrawable");
        _MTK_PRIVATE_DEF_SEL(setAutoResizeDrawable_, "setAutoResizeDrawable:");
        _MTK_PRIVATE_DEF_SEL(preferredFramesPerSecond, "preferredFramesPerSecond");
        _MTK_PRIVATE_DEF_SEL(setPreferredFramesPerSecond_,
            "setPreferredFramesPerSecond:");
        _MTK_PRIVATE_DEF_SEL(presentsWithTransaction, "presentsWithTransaction");
        _MTK_PRIVATE_DEF_SEL(setPresentsWithTransaction_,
            "setPresentsWithTransaction:");
        _MTK_PRIVATE_DEF_SEL(preferredDevice, "preferredDevice");
        _MTK_PRIVATE_DEF_SEL(preferredDrawableSize, "preferredDrawableSize");
        _MTK_PRIVATE_DEF_SEL(initWithFrame_device_, "initWithFrame:device:");
        _MTK_PRIVATE_DEF_SEL(setDevice_, "setDevice:");
        _MTK_PRIVATE_DEF_SEL(depthStencilTexture, "depthStencilTexture");
        _MTK_PRIVATE_DEF_SEL(multisampleColorTexture, "multisampleColorTexture");
        _MTK_PRIVATE_DEF_SEL(type, "type");
    } // namespace Selector
} // namespace Private
} // namespace MTK
