//
//  TextureLoader.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "TextureLoader.hpp"
#import <MetalKit/MetalKit.h>
#import <Foundation/Foundation.h>

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
}
}
