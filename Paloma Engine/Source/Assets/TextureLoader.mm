//
//  TextureLoader.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "TextureLoader.hpp"
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>
#import <ImageIO/ImageIO.h>

namespace Paloma{
namespace TextureLoader{

// -- Same options --
static NSDictionary* makeOptions(bool sRGB){
    return @{
        /// texture available in shaders
        MTKTextureLoaderOptionTextureUsage: @(MTLTextureUsageShaderRead),
        
        /// Private storage (only GPU)
        MTKTextureLoaderOptionTextureStorageMode: @(MTLStorageModePrivate),
        
        /// sRGB gamma correction
        MTKTextureLoaderOptionSRGB: @(sRGB)
    };
}

// -- Load from file --

MTL::Texture* loadFromFile(MTL::Device* device, const char* path, bool sRGB) {
    @autoreleasepool {
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        MTKTextureLoader* loader = [[MTKTextureLoader alloc] initWithDevice:mtlDevice];
        
        NSURL* url = [NSURL fileURLWithPath:[NSString stringWithUTF8String:path]];
        
        NSError* error = nil;
        id<MTLTexture> texture = [loader newTextureWithContentsOfURL:url options:makeOptions(sRGB) error:&error];
        
        if (!texture || error) {
            NSLog(@"TextureLoader: Failed to load %s: %@", path, error);
            return nullptr;
        }
        
        /// transfer ownership to C++
        return (__bridge_retained MTL::Texture*)texture;
    }
}

// -- Load from bundle --

MTL::Texture* loadFromBundle(MTL::Device* device, const char* name, bool sRGB) {
    @autoreleasepool {
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        MTKTextureLoader* loader = [[MTKTextureLoader alloc] initWithDevice:mtlDevice];
        
        NSString* textureName = [NSString stringWithUTF8String:name];
        
        NSError* error = nil;
        id<MTLTexture> texture = [loader newTextureWithName:textureName
                                                scaleFactor:1.0
                                                     bundle:nil
                                                    options:makeOptions(sRGB)
                                                      error:&error];
        
        if (!texture || error) {
            NSLog(@"TextureLoader: Failed to load %s from bundle: %@", name, error);
            return nullptr;
        }
        
        return (__bridge_retained MTL::Texture*)texture;
    }
}
MTL::Texture* loadHDR(MTL::Device* device, const char* path){
    @autoreleasepool {
        // Create URL from path
        NSString* nsPath = [NSString stringWithUTF8String:path];
        NSURL* url = [NSURL fileURLWithPath:nsPath];
        
        // Options for ImageIO
        NSDictionary* options = @{
            (__bridge NSString*)kCGImageSourceShouldCache: @NO,
            (__bridge NSString*)kCGImageSourceShouldAllowFloat: @YES
        };
        
        // Create CGImage descriptor
        CGImageSourceRef imageSource = CGImageSourceCreateWithURL(
                                                                  (__bridge CFURLRef)url,
                                                                  (__bridge CFDictionaryRef)options
                                                                  );
        
        if (!imageSource){
            NSLog(@"HDR: Failed to create image source for %s", path);
            return nullptr;
        }
        
        // Create CGImage
        CGImageRef image = CGImageSourceCreateImageAtIndex(imageSource, 0, NULL);
        CFRelease(imageSource); // release used descriptor
        
        if (!image){
            NSLog(@"HDR: Failed to create image from source");
            return nullptr;
        }
        
        // Get size
        size_t width = CGImageGetWidth(image);
        size_t height = CGImageGetHeight(image);
        
        size_t bitsPerPixel = CGImageGetBitsPerPixel(image);
        size_t bitsPerComponent = CGImageGetBitsPerComponent(image);
        size_t srcChannelCount = bitsPerPixel / bitsPerComponent;
        
        if (srcChannelCount != 3 && srcChannelCount != 4) {
            NSLog(@"HDR: Unexpected channel count: %zu", srcChannelCount);
            CFRelease(image);
            return nullptr;
        }
        
        // Get raw data
        CFDataRef imageData = CGDataProviderCopyData(CGImageGetDataProvider(image));
        const uint16_t* srcData = (const uint16_t*)CFDataGetBytePtr(imageData);
        
        // Alloc buffer for RGBA
        size_t pixelCount = width * height;
        size_t dstChannelCount = 4;
        size_t dstSize = pixelCount * dstChannelCount * sizeof(uint16_t);
        uint16_t* dstData = (uint16_t*)malloc(dstSize);
        
        // Copy with adding alpha
        for (size_t i = 0; i < pixelCount; ++i) {
            const uint16_t* src = srcData + (i * srcChannelCount);
            uint16_t* dst = dstData + (i * 4);
            dst[0] = src[0];  // R
            dst[1] = src[1];  // G
            dst[2] = src[2];  // B
            dst[3] = (srcChannelCount == 4) ? src[3] : 0x3C00;
        }
        
        // -- Create MTLTexture --
        
        id<MTLDevice> mtlDevice = (__bridge id<MTLDevice>)device;
        
        MTLTextureDescriptor* texDesc = [[MTLTextureDescriptor alloc]init];
        texDesc.pixelFormat = MTLPixelFormatRGBA16Float;
        texDesc.width = width;
        texDesc.height = height;
        texDesc.usage = MTLTextureUsageShaderRead;
        texDesc.storageMode = MTLStorageModeShared;
        
        // Create texture
        id<MTLTexture> texture = [mtlDevice newTextureWithDescriptor:texDesc];
        
        NSUInteger bytesPerRow = dstChannelCount * sizeof(uint16_t) * width;
        MTLRegion region = MTLRegionMake2D(0, 0, width, height);
        
        [texture replaceRegion:region mipmapLevel:0 withBytes:dstData bytesPerRow:bytesPerRow];
        
        free(dstData);
        CFRelease(imageData);
        CFRelease(image);
        
        // transfer ownership to Metal CPP
        return (__bridge_retained MTL::Texture*)texture;
    }
}

}
}
