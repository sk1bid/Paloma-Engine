/*
 * MTKTextureLoader.hpp
 */

#pragma once

#include "MetalKitPrivate.hpp"
#include <CoreGraphics/CGImage.h>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <ModelIO/ModelIO.hpp>

namespace MTK
{

using TextureLoaderOption = class NS::String*;

_NS_CONST(TextureLoaderOption, TextureLoaderOptionAllocateMipmaps);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionGenerateMipmaps);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionSRGB);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionTextureUsage);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionTextureCPUCacheMode);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionTextureStorageMode);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionCubeLayout);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionOrigin);
_NS_CONST(TextureLoaderOption, TextureLoaderOptionLoadAsArray);

_NS_CONST(TextureLoaderOption, TextureLoaderCubeLayoutVertical);

_NS_CONST(TextureLoaderOption, TextureLoaderOriginTopLeft);
_NS_CONST(TextureLoaderOption, TextureLoaderOriginBottomLeft);
_NS_CONST(TextureLoaderOption, TextureLoaderOriginFlippedVertically);

class TextureLoader : public NS::Referencing<MTK::TextureLoader>
{
public:
    static class TextureLoader* alloc();
    class TextureLoader*        init(MTL::Device* pDevice);

    MTL::Device*                device() const;

    // Synchronous methods
    MTL::Texture*               newTexture(const NS::URL* pURL, const NS::Dictionary* pOptions, NS::Error** ppError);
    MTL::Texture*               newTexture(const NS::Data* pData, const NS::Dictionary* pOptions, NS::Error** ppError);
    MTL::Texture*               newTexture(const MDL::Texture* pTexture, const NS::Dictionary* pOptions, NS::Error** ppError);
    MTL::Texture*               newTexture(CGImageRef cgImage, const NS::Dictionary* pOptions, NS::Error** ppError);
    MTL::Texture*               newTexture(const NS::String* pName, double scaleFactor, const NS::Bundle* pBundle, const NS::Dictionary* pOptions, NS::Error** ppError);

    NS::Array*                  newTextures(const NS::Array* pURLs, const NS::Dictionary* pOptions, NS::Error** ppError);
};
} // namespace MTK

// Implementations

_NS_INLINE MTK::TextureLoader* MTK::TextureLoader::alloc()
{
    return NS::Object::alloc<MTK::TextureLoader>(_MTK_PRIVATE_CLS(MTKTextureLoader));
}
_NS_INLINE MTK::TextureLoader* MTK::TextureLoader::init(MTL::Device* pDevice)
{
    return Object::sendMessage<MTK::TextureLoader*>(this, _MTK_PRIVATE_SEL(initWithDevice_), pDevice);
}
_NS_INLINE MTL::Device* MTK::TextureLoader::device() const
{
    return Object::sendMessage<MTL::Device*>(this, _MTK_PRIVATE_SEL(device));
}
_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture(const NS::URL* pURL, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(newTextureWithContentsOfURL_options_error_), pURL, pOptions, ppError);
}
_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture(const NS::Data* pData, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(newTextureWithData_options_error_), pData, pOptions, ppError);
}
_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture(const MDL::Texture* pTexture, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(newTextureWithMDLTexture_options_error_), pTexture, pOptions, ppError);
}
_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture(CGImageRef cgImage, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(newTextureWithCGImage_options_error_), cgImage, pOptions, ppError);
}
_NS_INLINE MTL::Texture* MTK::TextureLoader::newTexture(const NS::String* pName, double scaleFactor, const NS::Bundle* pBundle, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<MTL::Texture*>(this, _MTK_PRIVATE_SEL(newTextureWithName_scaleFactor_bundle_options_error_), pName, scaleFactor, pBundle, pOptions, ppError);
}
_NS_INLINE NS::Array* MTK::TextureLoader::newTextures(const NS::Array* pURLs, const NS::Dictionary* pOptions, NS::Error** ppError)
{
    return Object::sendMessage<NS::Array*>(this, _MTK_PRIVATE_SEL(newTexturesWithContentsOfURLs_options_error_), pURLs, pOptions, ppError);
}

#if defined(MTK_PRIVATE_IMPLEMENTATION)
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionAllocateMipmaps);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionGenerateMipmaps);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionSRGB);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionTextureUsage);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionTextureCPUCacheMode);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionTextureStorageMode);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionCubeLayout);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionOrigin);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOptionLoadAsArray);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderCubeLayoutVertical);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOriginTopLeft);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOriginBottomLeft);
_MTK_PRIVATE_DEF_CONST(NS::String*, TextureLoaderOriginFlippedVertically);
#endif
