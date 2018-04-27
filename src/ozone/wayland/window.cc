// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
#if defined(OS_WEBOS)
#include <sstream>
#endif

#include "ozone/wayland/window.h"

#include "base/logging.h"
#include "ozone/wayland/display.h"
#include "ozone/wayland/egl/egl_window.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell.h"
#include "ozone/wayland/shell/shell_surface.h"
#include "ui/platform_window/neva/window_group_configuration.h"

#if defined(OS_WEBOS)
#include "ozone/wayland/group/webos_surface_group.h"
#include "ozone/wayland/group/webos_surface_group_compositor.h"
#endif

namespace ozonewayland {

WaylandWindow::WaylandWindow(unsigned handle, int surface_id)
    : shell_surface_(NULL),
      window_(NULL),
      type_(None),
      handle_(handle),
      surface_id_(surface_id),
#if defined(OS_WEBOS)
      surface_group_(0),
      is_surface_group_client_(false),
#endif
      allocation_(gfx::Rect(0, 0, 1, 1)) {
}

WaylandWindow::~WaylandWindow() {
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  if (seat) {
    if (seat->GetFocusWindowHandle() == handle_)
      seat->SetFocusWindowHandle(0);

    if (seat->GetGrabWindowHandle() == handle_)
      seat->SetGrabWindowHandle(0, 0);
  }

  delete window_;
  delete shell_surface_;
}

void WaylandWindow::SetShellAttributes(ShellType type) {
  if (type_ == type)
    return;

  if (!shell_surface_) {
    shell_surface_ =
        WaylandDisplay::GetInstance()->GetShell()->CreateShellSurface(
            this, type, surface_id_);
  }

  type_ = type;
  shell_surface_->UpdateShellSurface(type_, NULL, 0, 0);
  allocation_.set_origin(gfx::Point(0, 0));
#if defined(OS_WEBOS)
  // Temporary set AppId in here. We should set AppId from app level.
  static int id = 0;
  id++;
  const char kAppId[] = "appId";
  const char kApp[] = "com.webos.app.";
  std::stringstream strs;
  strs << kApp << id;
  std::string kAppWindowId = strs.str();
  SetWindowProperty(kAppId, kAppWindowId);
#endif
}

void WaylandWindow::SetShellAttributes(ShellType type,
                                       WaylandShellSurface* shell_parent,
                                       int x,
                                       int y) {
  DCHECK(shell_parent && (type == POPUP));

  if (!shell_surface_) {
    shell_surface_ =
        WaylandDisplay::GetInstance()->GetShell()->CreateShellSurface(
            this, type, surface_id_);
  }

  type_ = type;
  shell_surface_->UpdateShellSurface(type_, shell_parent, x, y);
  allocation_.set_origin(gfx::Point(x, y));
}

void WaylandWindow::SetWindowTitle(const base::string16& title) {
  shell_surface_->SetWindowTitle(title);
}

void WaylandWindow::Maximize() {
  if (type_ != FULLSCREEN)
    shell_surface_->Maximize();
}

void WaylandWindow::Minimize() {
  shell_surface_->Minimize();
}

void WaylandWindow::Show() {
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  seat->SetFocusWindowHandle(handle_);
}

void WaylandWindow::Hide() {
  WaylandSeat* seat = WaylandDisplay::GetInstance()->PrimarySeat();
  if (seat && seat->GetFocusWindowHandle() == handle_)
    seat->SetFocusWindowHandle(0);
}

void WaylandWindow::Restore() {
  // If window is created as fullscreen, we don't set/restore any window states
  // like Maximize etc.
  if (type_ != FULLSCREEN)
    shell_surface_->UpdateShellSurface(type_, NULL, 0, 0);
}

void WaylandWindow::SetFullscreen() {
  if (type_ != FULLSCREEN)
    shell_surface_->UpdateShellSurface(FULLSCREEN, NULL, 0, 0);
}

void WaylandWindow::SetInputRegion(const std::vector<gfx::Rect>& region) {
  if (!shell_surface_) {
    LOG(ERROR) << "Shell type not set. Setting it to TopLevel";
    SetShellAttributes(TOPLEVEL);
  }

  shell_surface_->SetInputRegion(region);
}

void WaylandWindow::SetGroupKeyMask(ui::KeyMask key_mask) {
#if defined(OS_WEBOS)
  if (!shell_surface_) {
    LOG(ERROR) << "Shell type not set. Setting it to TopLevel";
    SetShellAttributes(TOPLEVEL);
  }

  shell_surface_->SetGroupKeyMask(key_mask);
#else
  LOG(INFO) << "WaylandWindow::SetGroupKeyMask() reached";
#endif
}

void WaylandWindow::SetKeyMask(ui::KeyMask key_mask, bool set) {
#if defined(OS_WEBOS)
  if (!shell_surface_) {
    LOG(ERROR) << "Shell type not set. Setting it to TopLevel";
    SetShellAttributes(TOPLEVEL);
  }

  shell_surface_->SetKeyMask(key_mask, set);
#else
  LOG(INFO) << "WaylandWindow::SetKeyMask() reached";
#endif
}

void WaylandWindow::SetWindowProperty(const std::string& name,
                                      const std::string& value) {
  if (!shell_surface_) {
    LOG(ERROR) << "Shell type not set. Setting it to TopLevel";
    SetShellAttributes(TOPLEVEL);
  }

  shell_surface_->SetWindowProperty(name, value);
}

void WaylandWindow::CreateGroup(const ui::WindowGroupConfiguration& config) {
#if defined(OS_WEBOS)
  DLOG_ASSERT(!surface_group_);
  WebOSSurfaceGroupCompositor* compositor =
      WaylandDisplay::GetInstance()->GetGroupCompositor();
  surface_group_ = compositor->CreateGroup(handle_, config.name);
  if (surface_group_) {
    surface_group_->SetAllowAnonymousLayers(config.is_anonymous);
    for (auto& it : config.layers) {
      surface_group_->CreateNamedLayer(it.name, it.z_order);
    }
  }
#else
  LOG(INFO) << "WaylandWindow::CreateGroup reached";
#endif
}

void WaylandWindow::AttachToGroup(const std::string& group,
                                  const std::string& layer) {
#if defined(OS_WEBOS)
  DLOG_ASSERT(!surface_group_);
  WebOSSurfaceGroupCompositor* compositor =
      WaylandDisplay::GetInstance()->GetGroupCompositor();
  surface_group_ = compositor->GetGroup(handle_, group);
  if (surface_group_) {
    surface_group_->AttachSurface(layer);
    is_surface_group_client_ = true;
    surface_group_client_layer_ = layer;
  }
#else
  LOG(INFO) << "WaylandWindow::AttachToGroup reached";
#endif
}

void WaylandWindow::FocusGroupOwner() {
#if defined(OS_WEBOS)
  if (surface_group_)
    surface_group_->FocusOwner();
#else
  LOG(INFO) << "WaylandWindow::FocusGroupOwner reached";
#endif
}

void WaylandWindow::FocusGroupLayer() {
#if defined(OS_WEBOS)
  if (surface_group_)
    surface_group_->FocusLayer(surface_group_client_layer_);
#else
  LOG(INFO) << "WaylandWindow::FocusGroupLayer reached";
#endif
}

void WaylandWindow::DetachGroup() {
#if defined(OS_WEBOS)
  if (!surface_group_)
    return;

  if (is_surface_group_client_) {
    surface_group_->DetachSurface();
    surface_group_client_layer_ = std::string();
  }

  delete surface_group_;
#else
  LOG(INFO) << "WaylandWindow::DetachGroup reached";
#endif
}

void WaylandWindow::RealizeAcceleratedWidget() {
  if (!shell_surface_) {
    LOG(ERROR) << "Shell type not set. Setting it to TopLevel";
    SetShellAttributes(TOPLEVEL);
  }

  if (!window_)
    window_ = new EGLWindow(shell_surface_->GetWLSurface(),
                            allocation_.width(),
                            allocation_.height());
}

void WaylandWindow::DestroyAcceleratedWidget() {
  delete window_;
  window_ = nullptr;
}

wl_egl_window* WaylandWindow::egl_window() const {
  DCHECK(window_);
  return window_->egl_window();
}

void WaylandWindow::Resize(unsigned width, unsigned height) {
  if ((allocation_.width() == int(width)) && (allocation_.height() == int(height)))
    return;

  allocation_ = gfx::Rect(allocation_.x(), allocation_.y(), width, height);
  if (!shell_surface_ || !window_)
    return;

  window_->Resize(allocation_.width(), allocation_.height());
  WaylandDisplay::GetInstance()->FlushDisplay();
}

void WaylandWindow::Move(ShellType type, WaylandShellSurface* shell_parent,
                         const gfx::Rect& rect) {
  int x = rect.x();
  int y = rect.y();
  if ((allocation_.x() == x) && (allocation_.y() == y))
    return;

  if (!shell_surface_ || !window_) {
    SetShellAttributes(type, shell_parent, x, y);
    return;
  }

  int move_x = x - allocation_.x();
  int move_y = y - allocation_.y();
  allocation_ = rect;
  window_->Move(allocation_.width(), allocation_.height(), move_x, move_y);
}

void WaylandWindow::AddRegion(int left, int top, int right, int bottom) {
  wl_compositor* com = WaylandDisplay::GetInstance()->GetCompositor();
  struct wl_region *region = wl_compositor_create_region(com);
  wl_region_add(region, left, top, right, bottom);
  wl_surface_set_input_region(shell_surface_->GetWLSurface(), region);
  wl_surface_set_opaque_region(shell_surface_->GetWLSurface(), region);
  wl_region_destroy(region);
}

void WaylandWindow::SubRegion(int left, int top, int right, int bottom) {
  wl_compositor* com = WaylandDisplay::GetInstance()->GetCompositor();
  struct wl_region *region = wl_compositor_create_region(com);
  wl_region_subtract(region, left, top, right, bottom);
  wl_surface_set_opaque_region(shell_surface_->GetWLSurface(), region);
  wl_region_destroy(region);
}

}  // namespace ozonewayland
