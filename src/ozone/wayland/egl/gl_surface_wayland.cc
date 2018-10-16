// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/wayland/egl/gl_surface_wayland.h"

#include "ozone/wayland/display.h"
#include "ozone/wayland/window.h"

#include <utility>

#include "third_party/khronos/EGL/egl.h"
#include "ui/ozone/common/egl_util.h"

namespace ozonewayland {

GLSurfaceWayland::GLSurfaceWayland(unsigned widget)
    : NativeViewGLSurfaceEGL(
          reinterpret_cast<EGLNativeWindowType>(
              WaylandDisplay::GetInstance()->GetEglWindow(widget)),
          nullptr),
      widget_(widget) {}

bool GLSurfaceWayland::Resize(const gfx::Size& size,
                              float scale_factor,
                              ColorSpace color_space,
                              bool has_alpha) {
  if (size_ == size)
    return true;

  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(widget_);
  DCHECK(window);
  window->Resize(size.width(), size.height());
  size_ = size;
  return true;
}

EGLConfig GLSurfaceWayland::GetConfig() {
  if (!config_) {
    GLint config_attribs[] = {EGL_BUFFER_SIZE,
                              32,
                              EGL_ALPHA_SIZE,
                              8,
                              EGL_BLUE_SIZE,
                              8,
                              EGL_GREEN_SIZE,
                              8,
                              EGL_RED_SIZE,
                              8,
                              EGL_RENDERABLE_TYPE,
                              EGL_OPENGL_ES2_BIT,
                              EGL_SURFACE_TYPE,
                              EGL_WINDOW_BIT,
                              EGL_NONE};
    config_ = ui::ChooseEGLConfig(GetDisplay(), config_attribs);
  }
  return config_;
}

gfx::SwapResult GLSurfaceWayland::SwapBuffers(
    const PresentationCallback& callback) {
  gfx::SwapResult result = NativeViewGLSurfaceEGL::SwapBuffers(callback);
  WaylandDisplay::GetInstance()->FlushDisplay();
  return result;
}

GLSurfaceWayland::~GLSurfaceWayland() {
  // Destroy surface first
  Destroy();
  // Then wl egl window if window instance is still around
  WaylandWindow* window = WaylandDisplay::GetInstance()->GetWindow(widget_);
  if (window) {
    window->DestroyAcceleratedWidget();
    WaylandDisplay::GetInstance()->FlushDisplay();
  }
}

}  // namespace ozonewayland
