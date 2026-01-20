//
//  main.cpp
//  Paloma Engine
//
//  Created by Artem on 19.01.2026.
//

#include <cassert>

#define NS_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION
#define MTK_PRIVATE_IMPLEMENTATION
#define MTLFX_PRIVATE_IMPLEMENTATION
#define MDL_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION

#include "AppKit/AppKit.hpp"
#include "Foundation/Foundation.hpp"
#include "Metal/Metal.hpp"
#include "MetalFX/MetalFX.hpp"
#include "MetalKit/MetalKit.hpp"
#include "ModelIO/ModelIO.hpp"
#include "QuartzCore/QuartzCore.hpp"

#include "Renderer.hpp"

class MyAppDelegate : public NS::ApplicationDelegate {
public:
  virtual ~MyAppDelegate() override { delete _pRenderer; }

  virtual void
  applicationDidFinishLaunching(NS::Notification *pNotification) override {
    CGRect frame = {{100, 100}, {1024, 768}};

    auto pDevice = NS::TransferPtr(MTL::CreateSystemDefaultDevice());

    _pWindow = NS::TransferPtr(NS::Window::alloc()->init(
        frame,
        NS::WindowStyleMaskTitled | NS::WindowStyleMaskClosable |
            NS::WindowStyleMaskResizable,
        NS::BackingStoreBuffered, false));

    _pRenderer = CreateRenderer(pDevice.get());

    auto pMtkView = MTK::View::alloc()->init(frame, pDevice.get());
    pMtkView->setDelegate(_pRenderer);

    _pRenderer->configure(pMtkView);

    _pWindow->setContentView(pMtkView);
    _pWindow->setTitle(
        NS::String::string("Paloma Engine", NS::UTF8StringEncoding));
    _pWindow->makeKeyAndOrderFront(nullptr);

    pMtkView->release();
  }

  virtual bool applicationShouldTerminateAfterLastWindowClosed(
      NS::Application *pSender) override {
    return true;
  }

private:
  NS::SharedPtr<NS::Window> _pWindow;
  RendererInterface *_pRenderer = nullptr;
};

int main(int argc, char *argv[]) {
  auto pPool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());

  MyAppDelegate delegate;
  auto pApp = NS::Application::sharedApplication();
  pApp->setDelegate(&delegate);
  pApp->run();

  return 0;
}
