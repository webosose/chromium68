// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_GL_SURFACE_WAYLAND_H_
#define UI_OZONE_PLATFORM_WAYLAND_GL_SURFACE_WAYLAND_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_surface_egl.h"
#include "ui/ozone/platform/wayland/wayland_object.h"

struct wl_callback;
struct wl_egl_window;

namespace ui {

class WaylandConnectionProxy;
class WaylandWindow;

struct EGLWindowDeleter {
  void operator()(wl_egl_window* egl_window);
};

std::unique_ptr<wl_egl_window, EGLWindowDeleter> CreateWaylandEglWindow(
    WaylandWindow* window);

// GLSurface class implementation for wayland.
class GLSurfaceWayland : public gl::NativeViewGLSurfaceEGL {
 public:
  using WaylandEglWindowPtr = std::unique_ptr<wl_egl_window, EGLWindowDeleter>;

  GLSurfaceWayland(WaylandEglWindowPtr egl_window,
                   WaylandWindow* window,
                   WaylandConnectionProxy* connection_);

  // gl::GLSurface:
  bool Resize(const gfx::Size& size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;
  EGLConfig GetConfig() override;
  bool SupportsAsyncSwap() override;
  void SwapBuffersAsync(
      const SwapCompletionCallback& completion_callback,
      const PresentationCallback& presentation_callback) override;

 private:
  ~GLSurfaceWayland() override;

  // wl_callback_listener
  static void FrameCallbackDone(void* data,
                                wl_callback* callback,
                                uint32_t time);

  // WaylandWindow, which holds a wl_surface which |egl_window_| is based on.
  WaylandWindow* window_ = nullptr;
  WaylandConnectionProxy* connection_ = nullptr;

  // A completion callback, which is called when a frame callback is received
  // from the compositor.
  SwapCompletionCallback completion_callback_;
  gfx::SwapResult swap_result_ = gfx::SwapResult::SWAP_FAILED;

  // A Wayland callback, which is triggered once wl_buffer has been committed
  // and it is right time to notify the GPU that it can start a new drawing
  // operation.
  wl::Object<wl_callback> wl_frame_callback_;

  WaylandEglWindowPtr egl_window_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceWayland);
};

}  // namespace ui
#endif  // UI_OZONE_PLATFORM_WAYLAND_GL_SURFACE_WAYLAND_H_
