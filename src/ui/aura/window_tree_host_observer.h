// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_AURA_WINDOW_TREE_HOST_OBSERVER_H_
#define UI_AURA_WINDOW_TREE_HOST_OBSERVER_H_

#include "ui/aura/aura_export.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

namespace gfx {
class Point;
}

namespace aura {
class WindowTreeHost;

class AURA_EXPORT WindowTreeHostObserver {
 public:
  // Called when the host's client size has changed.
  virtual void OnHostResized(WindowTreeHost* host) {}

  // Called when the host is moved on screen.
  virtual void OnHostMovedInPixels(WindowTreeHost* host,
                                   const gfx::Point& new_origin_in_pixels) {}

  // Called when the host is moved to a different workspace.
  virtual void OnHostWorkspaceChanged(WindowTreeHost* host) {}

  // Called when the native window system sends the host request to close.
  virtual void OnHostCloseRequested(WindowTreeHost* host) {}

  // Called when the accelerated widget is overridden for the host.
  virtual void OnAcceleratedWidgetOverridden(WindowTreeHost* host) {}

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  // Called when the host's state has changed.
  virtual void OnWindowHostStateChanged(WindowTreeHost* host, ui::WidgetState new_state) {}
  // Called when the host's window has been closed.
  virtual void OnWindowHostClose(WindowTreeHost* host) {}
#endif

 protected:
  virtual ~WindowTreeHostObserver() {}
};

}  // namespace aura

#endif  // UI_AURA_WINDOW_TREE_HOST_OBSERVER_H_
