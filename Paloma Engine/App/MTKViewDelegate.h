//
//  MTKViewDelegate.h
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import <MetalKit/MetalKit.h>

namespace Paloma {
class Context;
}

@interface PalomaViewDelegate : NSObject <MTKViewDelegate>
- (instancetype)initWithDevice:(id<MTLDevice>)device;
@end
