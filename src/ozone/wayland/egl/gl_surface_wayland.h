// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_
#define OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_

#include <memory>

#include "base/macros.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gl/gl_surface_egl.h"

namespace ozonewayland {

// GLSurface class implementation for wayland.
class GLSurfaceWayland : public gl::NativeViewGLSurfaceEGL {
 public:
  explicit GLSurfaceWayland(unsigned widget);

  // gl::GLSurface:
  bool Resize(const gfx::Size& size,
              float scale_factor,
              ColorSpace color_space,
              bool has_alpha) override;
  EGLConfig GetConfig() override;
  gfx::SwapResult SwapBuffers(const PresentationCallback& callback) override;

 private:
  ~GLSurfaceWayland() override;

  unsigned widget_;

  DISALLOW_COPY_AND_ASSIGN(GLSurfaceWayland);
};

}  // namespace ozonewayland
#endif  // OZONE_WAYLAND_EGL_GL_SURFACE_WAYLAND_H_
