//
//  InputBridge.mm
//  Paloma Engine
//
//  Objective-C++ bridge for input handling
//

#include "InputManager.hpp"
#include <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>

static bool g_mouseCaptured = false;

extern "C" void setMouseCaptured(bool captured) {
  g_mouseCaptured = captured;
  if (captured) {
    CGAssociateMouseAndMouseCursorPosition(false);
    [NSCursor hide];
  } else {
    CGAssociateMouseAndMouseCursorPosition(true);
    [NSCursor unhide];
  }
}

extern "C" bool isMouseCaptured() { return g_mouseCaptured; }

extern "C" void setupInputHandlers() {
  // Capture mouse on startup
  setMouseCaptured(true);

  // Keyboard events
  [NSEvent
      addLocalMonitorForEventsMatchingMask:NSEventMaskKeyDown
                                   handler:^NSEvent *(NSEvent *event) {
                                     if (event.keyCode == kVK_Escape) {
                                       setMouseCaptured(!g_mouseCaptured);
                                       return nil;
                                     }

                                     InputManager::shared().onKeyDown(
                                         event.keyCode);
                                     return nil;
                                   }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskKeyUp
                                        handler:^NSEvent *(NSEvent *event) {
                                          InputManager::shared().onKeyUp(
                                              event.keyCode);
                                          return nil;
                                        }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskFlagsChanged
                                        handler:^NSEvent *(NSEvent *event) {
                                          if (event.modifierFlags &
                                              NSEventModifierFlagShift) {
                                            InputManager::shared().onKeyDown(
                                                kVK_Shift);
                                          } else {
                                            InputManager::shared().onKeyUp(
                                                kVK_Shift);
                                          }
                                          return event;
                                        }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskMouseMoved
                                        handler:^NSEvent *(NSEvent *event) {
                                          if (g_mouseCaptured) {
                                            InputManager::shared().onMouseMoved(
                                                event.deltaX, event.deltaY);
                                          }
                                          return event;
                                        }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDragged
                                        handler:^NSEvent *(NSEvent *event) {
                                          if (g_mouseCaptured) {
                                            InputManager::shared().onMouseMoved(
                                                event.deltaX, event.deltaY);
                                          }
                                          return event;
                                        }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskRightMouseDragged
                                        handler:^NSEvent *(NSEvent *event) {
                                          if (g_mouseCaptured) {
                                            InputManager::shared().onMouseMoved(
                                                event.deltaX, event.deltaY);
                                          }
                                          return event;
                                        }];

  [NSEvent addLocalMonitorForEventsMatchingMask:NSEventMaskLeftMouseDown
                                        handler:^NSEvent *(NSEvent *event) {
                                          if (!g_mouseCaptured) {
                                            setMouseCaptured(true);
                                          }
                                          return event;
                                        }];
}
