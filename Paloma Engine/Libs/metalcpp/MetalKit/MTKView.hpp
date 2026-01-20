/*
 * MTKView.hpp
 */

#pragma once

#include <CoreGraphics/CGColorSpace.h>
#include <CoreGraphics/CGGeometry.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

#include "../AppKit/NSView.hpp"
#include "MetalKitPrivate.hpp"
#include <objc/runtime.h>

namespace MTK
{

class ViewDelegate
{
public:
    virtual ~ViewDelegate() = default;
    virtual void drawableSizeWillChange(class View* pView, CGSize size) = 0;
    virtual void drawInMTKView(class View* pView) = 0;
};

class View : public NS::Referencing<MTK::View, NS::View>
{
public:
    static class View*          alloc();
    class View*                 init(CGRect frameRect, MTL::Device* pDevice);

    class ViewDelegate*         delegate() const;
    void                        setDelegate(class ViewDelegate* pDelegate);

    MTL::Device*                device() const;
    void                        setDevice(MTL::Device* pDevice);

    CA::MetalDrawable*          currentDrawable() const;

    bool                        framebufferOnly() const;
    void                        setFramebufferOnly(bool framebufferOnly);

    MTL::TextureUsage           depthStencilAttachmentTextureUsage() const;
    void                        setDepthStencilAttachmentTextureUsage(MTL::TextureUsage usage);

    MTL::TextureUsage           multisampleColorAttachmentTextureUsage() const;
    void                        setMultisampleColorAttachmentTextureUsage(MTL::TextureUsage usage);

    bool                        presentsWithTransaction() const;
    void                        setPresentsWithTransaction(bool presentsWithTransaction);

    MTL::PixelFormat            colorPixelFormat() const;
    void                        setColorPixelFormat(MTL::PixelFormat colorPixelFormat);

    MTL::PixelFormat            depthStencilPixelFormat() const;
    void                        setDepthStencilPixelFormat(MTL::PixelFormat depthStencilPixelFormat);

    MTL::StorageMode            depthStencilStorageMode() const;
    void                        setDepthStencilStorageMode(MTL::StorageMode depthStencilStorageMode);

    NS::UInteger                sampleCount() const;
    void                        setSampleCount(NS::UInteger sampleCount);

    MTL::ClearColor             clearColor() const;
    void                        setClearColor(MTL::ClearColor clearColor);

    double                      clearDepth() const;
    void                        setClearDepth(double clearDepth);

    uint32_t                    clearStencil() const;
    void                        setClearStencil(uint32_t clearStencil);

    MTL::Texture*               depthStencilTexture() const;
    MTL::Texture*               multisampleColorTexture() const;

    void                        releaseDrawables();

    MTL::RenderPassDescriptor*  currentRenderPassDescriptor() const;
    MTL4::RenderPassDescriptor* currentMTL4RenderPassDescriptor() const;

    NS::Integer                 preferredFramesPerSecond() const;
    void                        setPreferredFramesPerSecond(NS::Integer preferredFramesPerSecond);

    bool                        enableSetNeedsDisplay() const;
    void                        setEnableSetNeedsDisplay(bool enableSetNeedsDisplay);

    bool                        autoResizeDrawable() const;
    void                        setAutoResizeDrawable(bool autoResizeDrawable);

    CGSize                      drawableSize() const;
    void                        setDrawableSize(CGSize drawableSize);

    CGSize                      preferredDrawableSize() const;
    MTL::Device*                preferredDevice() const;

    bool                        isPaused() const;
    void                        setPaused(bool paused);

    void                        draw();
};
} // namespace MTK

// Implementations

_NS_INLINE MTK::View* MTK::View::alloc()
{
    return NS::Object::alloc<MTK::View>(_MTK_PRIVATE_CLS(MTKView));
}
_NS_INLINE MTK::View* MTK::View::init(CGRect frameRect, MTL::Device* pDevice)
{
    return Object::sendMessage<MTK::View*>(this, _MTK_PRIVATE_SEL(initWithFrame_device_), frameRect, pDevice);
}
_NS_INLINE MTK::ViewDelegate* MTK::View::delegate() const
{
    return Object::sendMessage<MTK::ViewDelegate*>(this, _MTK_PRIVATE_SEL(delegate));
}
_NS_INLINE void MTK::View::setDelegate(MTK::ViewDelegate* pDelegate)
{
    NS::Value* pWrapper = NS::Value::value(pDelegate);
    pWrapper->retain();
    
    typedef void (*DrawFunction)(NS::Value*, SEL, void*);
    typedef void (*SizeChangeFunction)(NS::Value*, SEL, void*, CGSize);

    DrawFunction drawInMTKView = [](NS::Value* pSelf, SEL, void* pView) {
        auto pDel = reinterpret_cast<MTK::ViewDelegate*>(pSelf->pointerValue());
        pDel->drawInMTKView(reinterpret_cast<MTK::View*>(pView));
    };

    SizeChangeFunction sizeWillChange = [](NS::Value* pSelf, SEL, void* pView, CGSize size) {
        auto pDel = reinterpret_cast<MTK::ViewDelegate*>(pSelf->pointerValue());
        pDel->drawableSizeWillChange(reinterpret_cast<MTK::View*>(pView), size);
    };

    class_addMethod((Class)NS::Private::Class::s_kNSValue, _MTK_PRIVATE_SEL(drawInMTKView_), (IMP)drawInMTKView, "v@:@");
    class_addMethod((Class)NS::Private::Class::s_kNSValue, _MTK_PRIVATE_SEL(mtkView_drawableSizeWillChange_), (IMP)sizeWillChange, "v@:@{CGSize=dd}");

    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDelegate_), pWrapper);
}
_NS_INLINE MTL::Device* MTK::View::device() const
{
    return Object::sendMessage<MTL::Device*>(this, _MTK_PRIVATE_SEL(device));
}
_NS_INLINE void MTK::View::setDevice(MTL::Device* pDevice)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDevice_), pDevice);
}
_NS_INLINE CA::MetalDrawable* MTK::View::currentDrawable() const
{
    return Object::sendMessage<CA::MetalDrawable*>(this, _MTK_PRIVATE_SEL(currentDrawable));
}
_NS_INLINE bool MTK::View::framebufferOnly() const
{
    return Object::sendMessage<bool>(this, _MTK_PRIVATE_SEL(framebufferOnly));
}
_NS_INLINE void MTK::View::setFramebufferOnly(bool framebufferOnly)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setFramebufferOnly_), framebufferOnly);
}
_NS_INLINE MTL::TextureUsage MTK::View::depthStencilAttachmentTextureUsage() const
{
    return Object::sendMessage<MTL::TextureUsage>(this, _MTK_PRIVATE_SEL(depthStencilAttachmentTextureUsage));
}
_NS_INLINE void MTK::View::setDepthStencilAttachmentTextureUsage(MTL::TextureUsage usage)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDepthStencilAttachmentTextureUsage_), usage);
}
_NS_INLINE MTL::TextureUsage MTK::View::multisampleColorAttachmentTextureUsage() const
{
    return Object::sendMessage<MTL::TextureUsage>(this, _MTK_PRIVATE_SEL(multisampleColorAttachmentTextureUsage));
}
_NS_INLINE void MTK::View::setMultisampleColorAttachmentTextureUsage(MTL::TextureUsage usage)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setMultisampleColorAttachmentTextureUsage_), usage);
}
_NS_INLINE MTL::PixelFormat MTK::View::colorPixelFormat() const
{
    return Object::sendMessage<MTL::PixelFormat>(this, _MTK_PRIVATE_SEL(colorPixelFormat));
}
_NS_INLINE void MTK::View::setColorPixelFormat(MTL::PixelFormat colorPixelFormat)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setColorPixelFormat_), colorPixelFormat);
}
_NS_INLINE MTL::PixelFormat MTK::View::depthStencilPixelFormat() const
{
    return Object::sendMessage<MTL::PixelFormat>(this, _MTK_PRIVATE_SEL(depthStencilPixelFormat));
}
_NS_INLINE void MTK::View::setDepthStencilPixelFormat(MTL::PixelFormat depthStencilPixelFormat)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDepthStencilPixelFormat_), depthStencilPixelFormat);
}
_NS_INLINE MTL::StorageMode MTK::View::depthStencilStorageMode() const
{
    return Object::sendMessage<MTL::StorageMode>(this, _MTK_PRIVATE_SEL(depthStencilStorageMode));
}
_NS_INLINE void MTK::View::setDepthStencilStorageMode(MTL::StorageMode depthStencilStorageMode)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDepthStencilStorageMode_), depthStencilStorageMode);
}
_NS_INLINE NS::UInteger MTK::View::sampleCount() const
{
    return Object::sendMessage<NS::UInteger>(this, _MTK_PRIVATE_SEL(sampleCount));
}
_NS_INLINE void MTK::View::setSampleCount(NS::UInteger sampleCount)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setSampleCount_), sampleCount);
}
_NS_INLINE MTL::ClearColor MTK::View::clearColor() const
{
    return Object::sendMessage<MTL::ClearColor>(this, _MTK_PRIVATE_SEL(clearColor));
}
_NS_INLINE void MTK::View::setClearColor(MTL::ClearColor clearColor)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setClearColor_), clearColor);
}
_NS_INLINE double MTK::View::clearDepth() const
{
    return Object::sendMessage<double>(this, _MTK_PRIVATE_SEL(clearDepth));
}
_NS_INLINE void MTK::View::setClearDepth(double clearDepth)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setClearDepth_), clearDepth);
}
_NS_INLINE uint32_t MTK::View::clearStencil() const
{
    return Object::sendMessage<uint32_t>(this, _MTK_PRIVATE_SEL(clearStencil));
}
_NS_INLINE void MTK::View::setClearStencil(uint32_t clearStencil)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setClearStencil_), clearStencil);
}
_NS_INLINE MTL::Texture* MTK::View::depthStencilTexture() const
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(depthStencilTexture));
}
_NS_INLINE MTL::Texture* MTK::View::multisampleColorTexture() const
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(multisampleColorTexture));
}
_NS_INLINE void MTK::View::releaseDrawables()
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(releaseDrawables));
}
_NS_INLINE MTL::RenderPassDescriptor* MTK::View::currentRenderPassDescriptor() const
{
    return Object::sendMessage<MTL::RenderPassDescriptor*>(this, _MTK_PRIVATE_SEL(currentRenderPassDescriptor));
}
_NS_INLINE MTL4::RenderPassDescriptor* MTK::View::currentMTL4RenderPassDescriptor() const
{
    return Object::sendMessage<MTL4::RenderPassDescriptor*>(this, _MTK_PRIVATE_SEL(currentMTL4RenderPassDescriptor));
}
_NS_INLINE bool MTK::View::isPaused() const
{
    return Object::sendMessage<bool>(this, _MTK_PRIVATE_SEL(isPaused));
}
_NS_INLINE void MTK::View::setPaused(bool paused)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setPaused_), paused);
}
_NS_INLINE void MTK::View::draw()
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(draw));
}
_NS_INLINE bool MTK::View::autoResizeDrawable() const
{
    return Object::sendMessage<bool>(this, _MTK_PRIVATE_SEL(autoResizeDrawable));
}
_NS_INLINE void MTK::View::setAutoResizeDrawable(bool autoResizeDrawable)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setAutoResizeDrawable_), autoResizeDrawable);
}
_NS_INLINE CGSize MTK::View::drawableSize() const
{
    return Object::sendMessage<CGSize>(this, _MTK_PRIVATE_SEL(drawableSize));
}
_NS_INLINE void MTK::View::setDrawableSize(CGSize drawableSize)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setDrawableSize_), drawableSize);
}
_NS_INLINE CGSize MTK::View::preferredDrawableSize() const
{
    return Object::sendMessage<CGSize>(this, _MTK_PRIVATE_SEL(preferredDrawableSize));
}
_NS_INLINE MTL::Device* MTK::View::preferredDevice() const
{
    return Object::sendMessage<MTL::Device*>(this, _MTK_PRIVATE_SEL(preferredDevice));
}
_NS_INLINE bool MTK::View::presentsWithTransaction() const
{
    return Object::sendMessage<bool>(this, _MTK_PRIVATE_SEL(presentsWithTransaction));
}
_NS_INLINE void MTK::View::setPresentsWithTransaction(bool presentsWithTransaction)
{
    Object::sendMessage<void>(this, _MTK_PRIVATE_SEL(setPresentsWithTransaction_), presentsWithTransaction);
}
