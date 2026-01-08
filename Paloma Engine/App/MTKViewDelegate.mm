//
//  MTKViewDelegate.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//


#import "MTKViewDelegate.h"
#include "Renderer.hpp"
#include "SpheresScene.hpp"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

@implementation PalomaViewDelegate {
    Paloma::Renderer* _renderer;
    Paloma::SpheresScene* _scene;
    
    CFTimeInterval _lastFrameTime;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device {
    self = [super init];
    if (self) {
        MTL::Device* cppDevice = (__bridge MTL::Device*)device;
        
        // Create renderer
        _renderer = new Paloma::Renderer();
        if (!_renderer->init(cppDevice)) {
            NSLog(@"Failed to init Renderer!");
            delete _renderer;
            _renderer = nullptr;
            return nil;
        }
        
        // Create and setup scene
        _scene = new Paloma::SpheresScene();
        _scene->setup(_renderer);
        _renderer->setScene(_scene);
        
        _lastFrameTime = CACurrentMediaTime();
        NSLog(@"Paloma Engine initialized!");
    }
    return self;
}

- (void)dealloc {
    if (_scene) {
        _scene->cleanup();
        delete _scene;
        _scene = nullptr;
    }
    if (_renderer) {
        _renderer->shutdown();
        delete _renderer;
        _renderer = nullptr;
    }
}

- (void)drawInMTKView:(MTKView*)view {
    if (!_renderer || !_scene) return;
    
    // Calculate delta time
    CFTimeInterval now = CACurrentMediaTime();
    float dt = (float)(now - _lastFrameTime);
    _lastFrameTime = now;
    
    // Update scene
    _scene->update(dt);
    
    // Update camera aspect ratio
    CGSize size = view.drawableSize;
    _scene->camera().aspectRatio = size.width / size.height;
    
    // Get render pass
    MTL4RenderPassDescriptor* renderPassDesc = view.currentMTL4RenderPassDescriptor;
    id<CAMetalDrawable> drawable = view.currentDrawable;
    id<MTLResidencySet> layerResidency = ((CAMetalLayer*)view.layer).residencySet;
    
    if (!renderPassDesc || !drawable) return;
    
    // Render
    _renderer->render(
        (__bridge MTL4::RenderPassDescriptor*)renderPassDesc,
        (__bridge CA::MetalDrawable*)drawable,
        (__bridge MTL::ResidencySet*)layerResidency,
        size.width, size.height, dt
    );
}

- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size {
    // Handle resize if needed
}

@end
