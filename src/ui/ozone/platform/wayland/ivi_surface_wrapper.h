// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_WAYLAND_IVI_SURFACE_WRAPPER_H_
#define UI_OZONE_PLATFORM_WAYLAND_IVI_SURFACE_WRAPPER_H_

#include "ui/ozone/platform/wayland/xdg_surface_wrapper.h"

#include "base/macros.h"

struct ivi_surface;

namespace ui {

class WaylandConnection;
class WaylandWindow;

class IviSurfaceWrapper : public XDGSurfaceWrapper {
 public:
  IviSurfaceWrapper(WaylandWindow* wayland_window);
  ~IviSurfaceWrapper() override;

  bool Initialize(WaylandConnection* connection,
                  wl_surface* surface,
                  bool with_toplevel) override;
  void SetMaximized() override;
  void UnSetMaximized() override;
  void SetFullscreen() override;
  void UnSetFullscreen() override;
  void SetMinimized() override;
  void SurfaceMove(WaylandConnection* connection) override;
  void SurfaceResize(WaylandConnection* connection, uint32_t hittest) override;
  void SetTitle(const base::string16& title) override;
  void AckConfigure() override;
  void SetWindowGeometry(const gfx::Rect& bounds) override;

 private:
  // ivi_surface_listener
  static void HandleConfigure(void* data,
                              struct ivi_surface* shell_surface,
                              int32_t width,
                              int32_t height);  

  WaylandWindow* wayland_window_;
  
  // TODO(msisov): use wl::Object.
  ivi_surface* ivi_surface_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(IviSurfaceWrapper);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_WAYLAND_IVI_SURFACE_WRAPPER_H_
