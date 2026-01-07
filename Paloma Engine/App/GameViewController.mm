//
//  GameViewController.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "GameViewController.h"
#import "MTKViewDelegate.h"

@implementation GameViewController {
  PalomaViewDelegate *_renderer;
}

- (void)viewDidLoad {
  [super viewDidLoad];

  MTKView *view = (MTKView *)self.view;
  view.device = MTLCreateSystemDefaultDevice();

  if (!view.device) {
    NSLog(@"No Metal device!");
    return;
  }

  if (![view.device supportsFamily:MTLGPUFamilyMetal4]) {
    NSLog(@"Metal 4 not supported!");
    return;
  }

  NSLog(@"Metal device: %@", view.device.name);

  // Configure MTKView
  view.colorPixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
  view.depthStencilPixelFormat = MTLPixelFormatDepth32Float;
  view.clearColor = MTLClearColorMake(0.1, 0.1, 0.15, 1.0);
  view.preferredFramesPerSecond = 120;

  // Create our Metal 4 renderer
  _renderer = [[PalomaViewDelegate alloc] initWithDevice:view.device];
  if (!_renderer) {
    NSLog(@"Failed to create PalomaViewDelegate!");
    return;
  }

  // Initialize with current size (Apple pattern, line 35)
  [_renderer mtkView:view drawableSizeWillChange:view.drawableSize];

  view.delegate = _renderer;
  NSLog(@"PalomaViewDelegate connected!");
}

@end
