// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/shell/browser/shell_platform_data_aura.h"

#include "base/macros.h"
#include "content/shell/browser/shell.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/env.h"
#include "ui/aura/layout_manager.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/input_method_delegate.h"
#include "ui/base/ime/input_method_factory.h"
#include "ui/wm/core/default_activation_client.h"

#if !defined(USE_CBE)
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_window_parenting_client.h"
#endif

#if defined(USE_OZONE)
#include "ui/aura/screen_ozone.h"
#endif

namespace content {

namespace {

class FillLayout : public aura::LayoutManager {
 public:
  explicit FillLayout(aura::Window* root)
      : root_(root) {
  }

  ~FillLayout() override {}

 private:
  // aura::LayoutManager:
  void OnWindowResized() override {}

  void OnWindowAddedToLayout(aura::Window* child) override {
    child->SetBounds(root_->bounds());
  }

  void OnWillRemoveWindowFromLayout(aura::Window* child) override {}

  void OnWindowRemovedFromLayout(aura::Window* child) override {}

  void OnChildWindowVisibilityChanged(aura::Window* child,
                                      bool visible) override {}

  void SetChildBounds(aura::Window* child,
                      const gfx::Rect& requested_bounds) override {
    SetChildBoundsDirect(child, requested_bounds);
  }

  aura::Window* root_;

  DISALLOW_COPY_AND_ASSIGN(FillLayout);
};

}

ShellPlatformDataAura* Shell::platform_ = nullptr;

ShellPlatformDataAura::ShellPlatformDataAura(const gfx::Size& initial_size) {
  CHECK(aura::Env::GetInstance());

  // Setup global display::Screen singleton.
  if (!display::Screen::GetScreen()) {
#if defined(USE_OZONE)
    auto platform_screen = ui::OzonePlatform::GetInstance()->CreateScreen();
    if (platform_screen)
      screen_ = std::make_unique<aura::ScreenOzone>(std::move(platform_screen));
#endif  // defined(USE_OZONE)

    // Use aura::TestScreen for Ozone platforms that don't provide
    // PlatformScreen.
    // TODO(https://crbug.com/872339): Implement PlatformScreen for all
    // platforms and remove this code.
    if (!screen_) {
      // Some layout tests expect to be able to resize the window, so the screen
      // must be larger than the window.
      screen_.reset(
          aura::TestScreen::Create(gfx::ScaleToCeiledSize(initial_size, 2.0)));
    }
    display::Screen::SetScreenInstance(screen_.get());
  }

  host_.reset(aura::WindowTreeHost::Create(gfx::Rect(initial_size)));
  host_->InitHost();
  host_->window()->Show();
  host_->window()->SetLayoutManager(new FillLayout(host_->window()));

#if !defined(USE_CBE)
  focus_client_.reset(new aura::test::TestFocusClient());
  aura::client::SetFocusClient(host_->window(), focus_client_.get());
#endif

  new wm::DefaultActivationClient(host_->window());
  capture_client_.reset(
      new aura::client::DefaultCaptureClient(host_->window()));
#if !defined(USE_CBE)
  window_parenting_client_.reset(
      new aura::test::TestWindowParentingClient(host_->window()));
#endif
}

ShellPlatformDataAura::~ShellPlatformDataAura() {
  if (screen_)
    display::Screen::SetScreenInstance(nullptr);
}

void ShellPlatformDataAura::ShowWindow() {
  host_->Show();
}

void ShellPlatformDataAura::ResizeWindow(const gfx::Size& size) {
  host_->SetBoundsInPixels(gfx::Rect(size));
}

}  // namespace content
