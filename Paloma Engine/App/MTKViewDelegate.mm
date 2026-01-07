//
//  MTKViewDelegate.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "MTKViewDelegate.h"
#include "Bridge.hpp"
#include "Context.hpp"
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>

@implementation PalomaViewDelegate {
  Paloma::Context *_context;
}

- (void)drawInMTKView:(nonnull MTKView *)view {
  if (!_context)
    return;

  _context->beginFrame();

  auto renderPassDesc =
      Bridge::currentRenderPassDescriptor((__bridge void *)view);
  if (!renderPassDesc) {
    return; // Skip this frame if no descriptor
  }

  auto encoder =
      _context->commandBuffer()->renderCommandEncoder(renderPassDesc);
  encoder->endEncoding();

  auto layerResidency = Bridge::layerResidencySet((__bridge void *)view);

  auto drawable = Bridge::currentDrawable((__bridge void *)view);
  if (!drawable)
    return;

  _context->endFrame(drawable, layerResidency);
}

- (void)mtkView:(nonnull MTKView *)view drawableSizeWillChange:(CGSize)size {
  // Handle resize if needed
}

- (instancetype)initWithDevice:(id<MTLDevice>)device {
  self = [super init];
  if (self) {
    _context = new Paloma::Context();

    if (!_context->init()) {
      NSLog(@"Failed to init Metal 4 Context");
      delete _context;
      _context = nullptr;
      return nil;
    }

    NSLog(@"Metal 4 Context initialized");
  }
  return self;
}

- (void)dealloc {
  if (_context) {
    _context->shutdown();
    delete _context;
    _context = nullptr;
  }
}

@end
