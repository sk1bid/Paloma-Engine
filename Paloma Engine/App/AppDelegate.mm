//
//  AppDelegate.mm
//  Paloma Engine
//
//  Created by Artem on 07.01.2026.
//

#import "AppDelegate.h"

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)notification{
    NSLog(@"Paloma Engine started");
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender {
    return YES; /// close window = close app
}

@end
