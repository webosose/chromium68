// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/wayland/ivi_surface_wrapper.h"

#include <ivi-application-client-protocol.h>

#include "ui/ozone/platform/wayland/wayland_connection.h"
#include "ui/ozone/platform/wayland/wayland_util.h"
#include "ui/ozone/platform/wayland/wayland_window.h"

namespace ui {

IviSurfaceWrapper::IviSurfaceWrapper(WaylandWindow* wayland_window)
    : wayland_window_(wayland_window) {}

IviSurfaceWrapper::~IviSurfaceWrapper() {
  ivi_surface_destroy(ivi_surface_);
}

bool IviSurfaceWrapper::Initialize(WaylandConnection* connection,
                                     wl_surface* surface,
                                     bool with_toplevel) {
  static const struct ivi_surface_listener ivi_surface_listener = {
    &IviSurfaceWrapper::HandleConfigure,
  };

  int surface_id = wayland_window_->surface_id();
  // The window_manager on AGL handles surface_id 0 as an invalid id.
  if (surface_id == 0) {
    LOG(INFO) << __func__
              << " Using pid as long as surface_id is 0";
    surface_id = static_cast<int>(getpid());
  }

  ivi_surface_ = ivi_application_surface_create(connection->ivi_shell(),
                                                surface_id, surface);
  DCHECK(ivi_surface_);
  ivi_surface_add_listener(ivi_surface_, &ivi_surface_listener, this);
  return true;
}

void IviSurfaceWrapper::SetMaximized() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::UnSetMaximized() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SetFullscreen() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::UnSetFullscreen() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SetMinimized() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SurfaceMove(WaylandConnection* connection) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SurfaceResize(WaylandConnection* connection,
                                        uint32_t hittest) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SetTitle(const base::string16& title) {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::AckConfigure() {
  NOTIMPLEMENTED_LOG_ONCE();
}

void IviSurfaceWrapper::SetWindowGeometry(const gfx::Rect& bounds) {
  NOTIMPLEMENTED_LOG_ONCE();
}

// static
void IviSurfaceWrapper::HandleConfigure(void* data,
                              struct ivi_surface* shell_surface,
                              int32_t width,
                              int32_t height) { 
  IviSurfaceWrapper* surface = static_cast<IviSurfaceWrapper*>(data);
  surface->wayland_window_->HandleSurfaceConfigure(width, height,
                                                   false /* is_maximized */,
                                                   false /* is_fullscreen */,
                                                   true /* is_activated */);
}

}  // namespace ui
