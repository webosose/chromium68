// Copyright 2013 The Chromium Authors. All rights reserved.
// Copyright 2013 Intel Corporation. All rights reserved.
// Copyright (c) 2017-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#include "ozone/wayland/display.h"

#include <EGL/egl.h>
#include <errno.h>
#include <fcntl.h>
#if defined(ENABLE_DRM_SUPPORT)
#include <gbm.h>
#include <libdrm/drm.h>
#include <xf86drm.h>
#endif
#include <string>

#include "base/bind.h"
#include "base/files/file_path.h"
#include "base/logging_pmlog.h"
#include "base/message_loop/message_loop.h"
#include "base/native_library.h"
#include "base/stl_util.h"
#include "ipc/ipc_sender.h"
#include "ozone/platform/messages.h"
#if defined(USE_DATA_DEVICE_MANAGER)
#include "ozone/wayland/data_device.h"
#endif
#include "ozone/wayland/display_poll_thread.h"
#if defined(ENABLE_DRM_SUPPORT)
#include "ozone/wayland/egl/wayland_pixmap.h"
#endif
#include "ozone/wayland/input/cursor.h"
#if defined(OS_WEBOS)
#include "ozone/wayland/group/webos_surface_group_compositor.h"
#include "wayland-text-client-protocol.h"
#include "wayland-webos-extension-client-protocol.h"
#else
#include "ozone/wayland/protocol/text-client-protocol.h"
#endif
#if defined(ENABLE_DRM_SUPPORT)
#include "ozone/wayland/protocol/wayland-drm-protocol.h"
#endif
#include "ozone/wayland/screen.h"
#include "ozone/wayland/seat.h"
#include "ozone/wayland/shell/shell.h"
#include "ozone/wayland/window.h"
#include "ui/gfx/native_pixmap.h"
#include "ui/ozone/common/egl_util.h"
#include "ui/ozone/common/gl_ozone_egl.h"
#include "ui/ozone/public/surface_ozone_canvas.h"

#if defined(ENABLE_DRM_SUPPORT)
namespace {
using ozonewayland::WaylandDisplay;
// os-compatibility
static struct wl_drm* m_drm = 0;

void drm_handle_device(void* data, struct wl_drm*, const char* device) {
  WaylandDisplay* display = static_cast<WaylandDisplay*>(data);
  display->DrmHandleDevice(device);
}

void drm_handle_format(void* data, struct wl_drm*, uint32_t format) {
  WaylandDisplay* d = static_cast<WaylandDisplay*>(data);
  d->SetWLDrmFormat(format);
}

void drm_handle_authenticated(void* data, struct wl_drm*) {
  WaylandDisplay* d = static_cast<WaylandDisplay*>(data);
  d->DrmAuthenticated();
}

void drm_capabilities_(void* data, struct wl_drm*, uint32_t value) {
  struct WaylandDisplay* d = static_cast<WaylandDisplay*>(data);
  d->SetDrmCapabilities(value);
}

static const struct wl_drm_listener drm_listener = {
    drm_handle_device,
    drm_handle_format,
    drm_handle_authenticated
};

}  // namespace
#endif

namespace ozonewayland {

class GLOzoneEGLWayland : public ui::GLOzoneEGL {
 public:
  GLOzoneEGLWayland(WaylandDisplay* display) : display_(display) {}
  ~GLOzoneEGLWayland() override {}

  scoped_refptr<gl::GLSurface> CreateViewGLSurface(
      gfx::AcceleratedWidget widget) override;

  scoped_refptr<gl::GLSurface> CreateOffscreenGLSurface(
      const gfx::Size& size) override;

 protected:
  intptr_t GetNativeDisplay() override;
  bool LoadGLES2Bindings(gl::GLImplementation implementation) override;

 private:
  WaylandDisplay* display_;

  DISALLOW_COPY_AND_ASSIGN(GLOzoneEGLWayland);
};

scoped_refptr<gl::GLSurface> GLOzoneEGLWayland::CreateViewGLSurface(
    gfx::AcceleratedWidget widget) {
  return gl::InitializeGLSurface(new GLSurfaceWayland(widget));
}

scoped_refptr<gl::GLSurface> GLOzoneEGLWayland::CreateOffscreenGLSurface(
    const gfx::Size& size) {
  if (gl::GLSurfaceEGL::IsEGLSurfacelessContextSupported() &&
      size.width() == 0 && size.height() == 0) {
    return gl::InitializeGLSurface(new gl::SurfacelessEGL(size));
  } else {
    return gl::InitializeGLSurface(new gl::PbufferGLSurfaceEGL(size));
  }
}

intptr_t GLOzoneEGLWayland::GetNativeDisplay() {
  return reinterpret_cast<intptr_t>(display_->display());
}

bool GLOzoneEGLWayland::LoadGLES2Bindings(gl::GLImplementation implementation) {
  if (!display_->display())
    return false;
  setenv("EGL_PLATFORM", "wayland", 0);
  return ui::LoadDefaultEGLGLES2Bindings(implementation);
}

WaylandDisplay* WaylandDisplay::instance_ = NULL;

WaylandDisplay::WaylandDisplay()
    : SurfaceFactoryOzone(),
      display_(NULL),
      registry_(NULL),
      compositor_(NULL),
      data_device_manager_(NULL),
      shell_(NULL),
      shm_(NULL),
      primary_screen_(NULL),
      primary_seat_(NULL),
      display_poll_thread_(NULL),
#if defined(ENABLE_DRM_SUPPORT)
      device_(NULL),
      m_deviceName(NULL),
#endif
      sender_(NULL),
      loop_(NULL),
      screen_list_(),
      seat_list_(),
      widget_map_(),
      serial_(0),
      processing_events_(false),
#if defined(ENABLE_DRM_SUPPORT)
      m_authenticated_(false),
      m_fd_(-1),
      m_capabilities_(0),
#endif
      egl_implementation_(new GLOzoneEGLWayland(this)),
      weak_ptr_factory_(this) {
}

WaylandDisplay::~WaylandDisplay() {
  Terminate();
}

const std::list<WaylandScreen*>& WaylandDisplay::GetScreenList() const {
  return screen_list_;
}

WaylandWindow* WaylandDisplay::GetWindow(unsigned window_handle) const {
  return GetWidget(window_handle);
}

#if defined(OS_WEBOS)
text_model_factory* WaylandDisplay::GetTextModelFactory() const {
  return text_model_factory_;
}

WebOSSurfaceGroupCompositor* WaylandDisplay::GetGroupCompositor() const {
  return group_compositor_.get();
}
#else
struct wl_text_input_manager* WaylandDisplay::GetTextInputManager() const {
  return text_input_manager_;
}
#endif

void WaylandDisplay::FlushDisplay() {
  wl_display_flush(display_);
}

void WaylandDisplay::DestroyWindow(unsigned w) {
  widget_map_.erase(w);
}

intptr_t WaylandDisplay::GetNativeWindow(unsigned window_handle) {
  WaylandWindow* widget = GetWidget(window_handle);
  DCHECK(widget);
  widget->RealizeAcceleratedWidget();

  PMLOG_INFO(Ozone, "WaylandDisplay",
             "GetNativeWindow(id:%d widget:%p egl:%p) at %s(%d)", window_handle,
             widget, widget ? widget->egl_window() : 0, __FUNCTION__, __LINE__);

  return reinterpret_cast<intptr_t>(widget->egl_window());
}

wl_egl_window* WaylandDisplay::GetEglWindow(
    unsigned window_handle) {
  WaylandWindow* widget = GetWidget(window_handle);
  DCHECK(widget);
  widget->RealizeAcceleratedWidget();
  return widget->egl_window();
}

bool WaylandDisplay::InitializeHardware() {
  InitializeDisplay();
  if (!display_) {
    LOG(ERROR) << "WaylandDisplay failed to initialize hardware";
    return false;
  }

  // Ensure we are processing wayland event requests. This needs to be done here
  // so we start polling before sandbox is initialized.
  StartProcessingEvents();
  return true;
}

scoped_refptr<gfx::NativePixmap> WaylandDisplay::CreateNativePixmap(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    gfx::BufferUsage usage) {
#if defined(ENABLE_DRM_SUPPORT)
  if (usage == MAP)
    return NULL;

  scoped_refptr<WaylandPixmap> pixmap(new WaylandPixmap());
  if (!pixmap->Initialize(device_, format, size))
    return NULL;

  return pixmap;
#else
  return SurfaceFactoryOzone::CreateNativePixmap(widget, size, format, usage);
#endif
}

scoped_refptr<gfx::NativePixmap> WaylandDisplay::CreateNativePixmapFromHandle(
    gfx::AcceleratedWidget widget,
    gfx::Size size,
    gfx::BufferFormat format,
    const gfx::NativePixmapHandle& handle) {
  NOTIMPLEMENTED();
  return nullptr;
}

std::unique_ptr<ui::SurfaceOzoneCanvas> WaylandDisplay::CreateCanvasForWidget(
    gfx::AcceleratedWidget widget) {
  LOG(FATAL) << "The browser process has attempted to start the GPU process in "
             << "software rendering mode. Software rendering is not supported "
             << "in Ozone-Wayland, so this is fatal. Usually this error occurs "
             << "because the GPU process crashed in hardware rendering mode, "
             << "often due to failure to initialize EGL. To debug the GPU "
             << "process, start Chrome with --gpu-startup-dialog so that the "
             << "GPU process pauses on startup, then attach to it with "
             << "'gdb -p' and run the command 'signal SIGUSR1' in order to "
             << "unpause it. If you have xterm then it is easier to run "
             << "'chrome --no-sandbox --gpu-launcher='xterm -title renderer "
             << "-e gdb --eval-command=run --args''";

  // This code will obviously never be reached, but it placates -Wreturn-type.
  return std::unique_ptr<ui::SurfaceOzoneCanvas>();
}

std::vector<gl::GLImplementation>
WaylandDisplay::GetAllowedGLImplementations() {
  std::vector<gl::GLImplementation> impls;
  impls.push_back(gl::kGLImplementationEGLGLES2);
  return impls;
}

ui::GLOzone* WaylandDisplay::GetGLOzone(
    gl::GLImplementation implementation) {
  switch (implementation) {
    case gl::kGLImplementationEGLGLES2:
      return egl_implementation_.get();
    default:
      return nullptr;
  }
}

void WaylandDisplay::InitializeDisplay() {
  DCHECK(!display_);
  display_ = wl_display_connect(NULL);
  if (!display_)
    return;

  instance_ = this;
  static const struct wl_registry_listener registry_all = {
    WaylandDisplay::DisplayHandleGlobal
  };

  registry_ = wl_display_get_registry(display_);
  wl_registry_add_listener(registry_, &registry_all, this);
  shell_ = new WaylandShell();

  if (wl_display_roundtrip(display_) < 0) {
    Terminate();
    return;
  }

  display_poll_thread_ = new WaylandDisplayPollThread(display_);
}

WaylandWindow* WaylandDisplay::CreateAcceleratedSurface(unsigned w) {
  WaylandWindow* window = new WaylandWindow(w);
  widget_map_[w].reset(window);

  PMLOG_INFO(Ozone, "WaylandDisplay",
             "Wayland Window(id:%d widget:%p) is created. at %s(%d)", w, window,
             __FUNCTION__, __LINE__);

  return window;
}

void WaylandDisplay::StartProcessingEvents() {
  DCHECK(display_poll_thread_);
  // Start polling for wayland events.
  if (!processing_events_) {
    display_poll_thread_->StartProcessingEvents();
    processing_events_ = true;
  }
}

void WaylandDisplay::StopProcessingEvents() {
  DCHECK(display_poll_thread_);
  // Start polling for wayland events.
  if (processing_events_) {
    display_poll_thread_->StopProcessingEvents();
    processing_events_ = false;
  }
}

void WaylandDisplay::Terminate() {
  loop_ = NULL;
  if (!widget_map_.empty()) {
    widget_map_.clear();
  }

  for (WaylandSeat* seat : seat_list_)
    delete seat;

  for (WaylandScreen* screen : screen_list_)
    delete screen;

  screen_list_.clear();
  seat_list_.clear();

#if defined(OS_WEBOS)
  if (text_model_factory_)
    text_model_factory_destroy(text_model_factory_);
#else
  if (text_input_manager_)
    wl_text_input_manager_destroy(text_input_manager_);
#endif

#if defined(USE_DATA_DEVICE_MANAGER)
  if (data_device_manager_)
    wl_data_device_manager_destroy(data_device_manager_);
#endif

#if defined(ENABLE_DRM_SUPPORT)
  if (m_deviceName)
    delete m_deviceName;

  if (m_drm) {
    wl_drm_destroy(m_drm);
    m_drm = NULL;
  }

  close(m_fd_);
#endif
  if (compositor_)
    wl_compositor_destroy(compositor_);

  delete shell_;
  if (shm_)
    wl_shm_destroy(shm_);

  if (registry_)
    wl_registry_destroy(registry_);

  delete display_poll_thread_;

  if (display_) {
    wl_display_flush(display_);
    wl_display_disconnect(display_);
    display_ = NULL;
  }

  while (!deferred_messages_.empty())
    deferred_messages_.pop();

  instance_ = NULL;
}

WaylandWindow* WaylandDisplay::GetWidget(unsigned w) const {
  WindowMap::const_iterator it = widget_map_.find(w);
  return it == widget_map_.end() ? NULL : it->second.get();
}

void WaylandDisplay::SetWidgetState(unsigned w, ui::WidgetState state) {
  switch (state) {
    case ui::WidgetState::FULLSCREEN:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->SetFullscreen();
      break;
    }
    case ui::WidgetState::MAXIMIZED:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->Maximize();
      break;
    }
    case ui::WidgetState::MINIMIZED:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->Minimize();
      break;
    }
    case ui::WidgetState::RESTORE:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->Restore();
      break;
    }
    case ui::WidgetState::ACTIVE:
      NOTIMPLEMENTED() << " ACTIVE " << w;
      break;
    case ui::WidgetState::INACTIVE:
      NOTIMPLEMENTED() << " INACTIVE " << w;
      break;
    case ui::WidgetState::SHOW:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->Show();
      break;
    }
    case ui::WidgetState::HIDE:
    {
      WaylandWindow* widget = GetWidget(w);
      widget->Hide();
      break;
    }
    default:
      break;
  }
}

void WaylandDisplay::SetWidgetTitle(unsigned w, const base::string16& title) {
  WaylandWindow* widget = GetWidget(w);
  DCHECK(widget);
  widget->SetWindowTitle(title);
}

void WaylandDisplay::CreateWidget(unsigned widget) {
  DCHECK(!GetWidget(widget));
  CreateAcceleratedSurface(widget);
}

void WaylandDisplay::InitWindow(unsigned handle,
                                unsigned parent,
                                const gfx::Rect& rect,
                                ui::WidgetType type,
                                int surface_id) {
#if defined(OS_WEBOS)
  PointerVisibilityNotify(GetPointerCursorVisible());
#endif

  WaylandWindow* window = GetWidget(handle);

  window->SetSurfaceId(surface_id);
  WaylandWindow* parent_window = GetWidget(parent);
  DCHECK(window);
  switch (type) {
  case ui::WidgetType::WINDOW:
  case ui::WidgetType::WINDOWFRAMELESS:
    window->SetShellAttributes(WaylandWindow::TOPLEVEL);
    window->Resize(rect.width(), rect.height());
    break;
  case ui::WidgetType::POPUP:
  case ui::WidgetType::TOOLTIP:
    DCHECK(parent_window);
    window->SetShellAttributes(WaylandWindow::POPUP,
                               parent_window->ShellSurface(), rect.x(),
                               rect.y());
    break;
  default:
    break;
  }
}

void WaylandDisplay::MoveWindow(unsigned widget,
                                unsigned parent,
                                ui::WidgetType type,
                                const gfx::Rect& rect) {
  WaylandWindow::ShellType shell_type = WaylandWindow::None;
  switch (type) {
  case ui::WidgetType::WINDOW:
    shell_type = WaylandWindow::TOPLEVEL;
    break;
  case ui::WidgetType::WINDOWFRAMELESS:
    NOTIMPLEMENTED();
    break;
  case ui::WidgetType::POPUP:
  case ui::WidgetType::TOOLTIP:
    shell_type = WaylandWindow::POPUP;
    break;
  default:
    break;
  }

  WaylandWindow* popup = GetWidget(widget);
  WaylandShellSurface* shell_parent  = NULL;
  WaylandWindow* parent_window  = NULL;
  if (parent) {
    parent_window = GetWidget(parent);
    shell_parent = parent_window->ShellSurface();
  }
  popup->Move(shell_type, shell_parent, rect);
}

void WaylandDisplay::AddRegion(unsigned handle, int left, int top,
                               int right, int bottom) {
  WaylandWindow* widget = GetWidget(handle);
  DCHECK(widget);
  widget->AddRegion(left, top, right, bottom);
}

void WaylandDisplay::SubRegion(unsigned handle, int left, int top,
                               int right, int bottom) {
  WaylandWindow* widget = GetWidget(handle);
  DCHECK(widget);
  widget->SubRegion(left, top, right, bottom);
}

void WaylandDisplay::SetCursorBitmap(const std::vector<SkBitmap>& bitmaps,
                                     const gfx::Point& location) {
  primary_seat_->SetCursorBitmap(bitmaps, location);
}

void WaylandDisplay::MoveCursor(const gfx::Point& location) {
  primary_seat_->MoveCursor(location);
}

void WaylandDisplay::ResetIme() {
  primary_seat_->ResetIme();
}

void WaylandDisplay::ImeCaretBoundsChanged(gfx::Rect rect) {
  primary_seat_->ImeCaretBoundsChanged(rect);
}

void WaylandDisplay::ShowInputPanel(unsigned handle) {
  primary_seat_->ShowInputPanel(handle);
}

void WaylandDisplay::HideInputPanel() {
  primary_seat_->HideInputPanel();
}

void WaylandDisplay::SetInputContentType(ui::InputContentType content_type,
                                         int text_input_flags,
                                         unsigned handle) {
  primary_seat_->SetInputContentType(content_type, text_input_flags, handle);
}

void WaylandDisplay::RequestDragData(const std::string& mime_type) {
#if defined(USE_DATA_DEVICE_MANAGER)
  primary_seat_->GetDataDevice()->RequestDragData(mime_type);
#endif
}

void WaylandDisplay::RequestSelectionData(const std::string& mime_type) {
#if defined(USE_DATA_DEVICE_MANAGER)
  primary_seat_->GetDataDevice()->RequestSelectionData(mime_type);
#endif
}

void WaylandDisplay::DragWillBeAccepted(uint32_t serial,
                                        const std::string& mime_type) {
#if defined(USE_DATA_DEVICE_MANAGER)
  primary_seat_->GetDataDevice()->DragWillBeAccepted(serial, mime_type);
#endif
}

void WaylandDisplay::DragWillBeRejected(uint32_t serial) {
#if defined(USE_DATA_DEVICE_MANAGER)
  primary_seat_->GetDataDevice()->DragWillBeRejected(serial);
#endif
}

void WaylandDisplay::SetWindowProperty(unsigned w,
                                       const std::string& name,
                                       const std::string& value) {
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->SetWindowProperty(name, value);
}

void WaylandDisplay::DetachWindowGroup(unsigned w) {
#if defined(OS_WEBOS)
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->DetachGroup();
#else
  LOG(INFO) << __func__ << " reached";
#endif
}

void WaylandDisplay::CreateWindowGroup(
    unsigned w,
    const ui::WindowGroupConfiguration& config) {
#if defined(OS_WEBOS)
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->CreateGroup(config);
#else
  LOG(INFO) << __func__ << " reached";
#endif
}

void WaylandDisplay::AttachToWindowGroup(unsigned w,
                                         const std::string& group,
                                         const std::string& layer) {
#if defined(OS_WEBOS)
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->AttachToGroup(group, layer);
#else
  LOG(INFO) << __func__ << " reached";
#endif
}

void WaylandDisplay::FocusWindowGroupOwner(unsigned w) {
#if defined(OS_WEBOS)
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->FocusGroupOwner();
#else
  LOG(INFO) << __func__ << " reached";
#endif
}

void WaylandDisplay::FocusWindowGroupLayer(unsigned w) {
#if defined(OS_WEBOS)
  WaylandWindow* widget = GetWidget(w);
  if (!widget) {
    LOG(ERROR) << __PRETTY_FUNCTION__ << ", window not found.";
    return;
  }
  widget->FocusGroupLayer();
#endif
}

#if defined(ENABLE_DRM_SUPPORT)
void WaylandDisplay::DrmHandleDevice(const char* device) {
  drm_magic_t magic;
  m_deviceName = strdup(device);

  if (!m_deviceName)
    return;
  int flags = O_WRONLY | O_CREAT | O_TRUNC | O_CLOEXEC;
#ifdef O_CLOEXEC
  m_fd_ = open(device, flags, 0644);
  if (m_fd_ == -1 && errno == EINVAL) {
#endif
    m_fd_ = open(m_deviceName, flags, 0644);
    if (m_fd_ != -1)
      fcntl(m_fd_, F_SETFD, fcntl(m_fd_, F_GETFD) | FD_CLOEXEC);
#ifdef O_CLOEXEC
  }
#endif
  if (m_fd_ == -1) {
    LOG(ERROR) << "WaylandDisplay: could not open" << m_deviceName
        << strerror(errno);
    return;
  }

  drmGetMagic(m_fd_, &magic);
  wl_drm_authenticate(m_drm, magic);
}

void WaylandDisplay::SetWLDrmFormat(uint32_t) {
}

void WaylandDisplay::DrmAuthenticated() {
  m_authenticated_ = true;
  device_ = gbm_create_device(m_fd_);
  if (!device_) {
    LOG(ERROR) << "WaylandDisplay: Failed to create GBM Device.";
    close(m_fd_);
    m_fd_ = -1;
  }
}

void WaylandDisplay::SetDrmCapabilities(uint32_t value) {
  m_capabilities_ = value;
}
#endif

#if defined(OS_WEBOS)
void WaylandDisplay::InputPanelRectChanged(unsigned handle,
                                           int32_t x,
                                           int32_t y,
                                           uint32_t width,
                                           uint32_t height) {
  Dispatch(new WaylandInput_InputPanelRectChanged(handle, x, y, width, height));
}

void WaylandDisplay::InputPanelStateChanged(unsigned handle,
                                            webos::InputPanelState state) {
  switch(state) {
    case webos::INPUT_PANEL_SHOWN:
      InputPanelVisibilityChanged(handle, true);
      break;
    case webos::INPUT_PANEL_HIDDEN:
      InputPanelVisibilityChanged(handle, false);
  }
}

void WaylandDisplay::TextInputModifier(uint32_t state, uint32_t modifier) {
  NOTIMPLEMENTED();
}

void WaylandDisplay::OnWebosInputPointerListener(
    void* data,
    wl_webos_input_manager* wl_Webos_input_manager,
    uint32_t visible,
    wl_webos_seat* changed_webos_seat) {
  WaylandDisplay* disp = static_cast<WaylandDisplay*>(data);
  disp->PointerVisibilityNotify(visible);
  disp->SetPointerCursorVisible(visible);
}

void WaylandDisplay::PointerVisibilityNotify(bool visible) {
  Dispatch(new WaylandInput_CursorVisibilityChange(visible));
}
#endif

void WaylandDisplay::SetInputRegion(unsigned handle,
                                    const std::vector<gfx::Rect>& region) {
  WaylandWindow* widget = GetWidget(handle);

  if (!widget) {
    LOG(ERROR) << __func__ << "(): invalid window handle";
    return;
  }

  widget->SetInputRegion(region);
}

void WaylandDisplay::SetGroupKeyMask(unsigned handle, ui::KeyMask key_mask) {
  WaylandWindow* widget = GetWidget(handle);

  if (!widget) {
    LOG(ERROR) << __func__ << "(): invalid window handle";
    return;
  }

  widget->SetGroupKeyMask(key_mask);
}

void WaylandDisplay::SetKeyMask(unsigned handle,
                                ui::KeyMask key_mask,
                                bool set) {
  WaylandWindow* widget = GetWidget(handle);

  if (!widget) {
    LOG(ERROR) << __func__ << "(): invalid window handle";
    return;
  }

  widget->SetKeyMask(key_mask, set);
}

void WaylandDisplay::SetSurroundingText(const std::string& text,
                                        size_t cursor_position,
                                        size_t anchor_position) {
  primary_seat_->SetSurroundingText(text, cursor_position, anchor_position);
}

void WaylandDisplay::XInputActivate(const std::string& type) {
#if defined(OS_WEBOS)
  if (!webos_xinput_)
    return;

  wl_webos_xinput_activated(webos_xinput_, type.c_str());
#else
  LOG(INFO) << "WaylandDisplay::XInputActivate reached";
#endif
}

void WaylandDisplay::XInputDeactivate() {
#if defined(OS_WEBOS)
  if (!webos_xinput_)
    return;

  wl_webos_xinput_deactivated(webos_xinput_);
#else
  LOG(INFO) << "WaylandDisplay::XInputDeactivate reached";
#endif
}

void WaylandDisplay::XInputInvokeAction(uint32_t keysym,
                                        ui::XInputKeySymbolType symbol_type,
                                        ui::XInputEventType event_type) {
#if defined(OS_WEBOS)
  if (!webos_xinput_)
    return;
  wl_webos_xinput_keysym_type wl_keysym_type;
  switch (symbol_type) {
    case ui::XINPUT_QT_KEY_SYMBOL:
      wl_keysym_type = WL_WEBOS_XINPUT_KEYSYM_TYPE_QT;
      break;
    case ui::XINPUT_NATIVE_KEY_SYMBOL:
      wl_keysym_type = WL_WEBOS_XINPUT_KEYSYM_TYPE_NATIVE;
      break;
    default:
      NOTREACHED();
      wl_keysym_type = WL_WEBOS_XINPUT_KEYSYM_TYPE_QT;
  }

  wl_webos_xinput_event_type wl_event_type;
  switch (event_type) {
    case ui::XINPUT_PRESS_AND_RELEASE:
      wl_event_type = WL_WEBOS_XINPUT_EVENT_TYPE_PRESS_AND_RELEASE;
      break;
    case ui::XINPUT_PRESS:
      wl_event_type = WL_WEBOS_XINPUT_EVENT_TYPE_PRESS;
      break;
    case ui::XINPUT_RELEASE:
      wl_event_type = WL_WEBOS_XINPUT_EVENT_TYPE_RELEASE;
      break;
    default:
      NOTREACHED();
      wl_event_type = WL_WEBOS_XINPUT_EVENT_TYPE_PRESS_AND_RELEASE;
  }
  wl_webos_xinput_invoke_action(webos_xinput_, keysym, wl_keysym_type,
                                wl_event_type);
#else
  LOG(INFO) << "WaylandDisplay::XInputInvokeAction reached";
#endif
}

// Additional notification for app-runtime
void WaylandDisplay::InputPanelVisibilityChanged(unsigned handle, bool visibility) {
  Dispatch(new WaylandInput_InputPanelVisibilityChanged(handle, visibility));
}

void WaylandDisplay::NativeWindowExposed(unsigned handle) {
  Dispatch(new WaylandWindow_Exposed(handle));
}

void WaylandDisplay::NativeWindowStateChanged(unsigned handle,
                                              ui::WidgetState new_state) {
  Dispatch(new WaylandWindow_StateChanged(handle, new_state));
}

void WaylandDisplay::NativeWindowStateAboutToChange(unsigned handle,
                                                    ui::WidgetState state) {
  Dispatch(new WaylandWindow_StateAboutToChange(handle, state));
}

void WaylandDisplay::WindowClose(unsigned handle) {
  Dispatch(new WaylandWindow_Close(handle));
}

void WaylandDisplay::KeyboardEnter(unsigned handle) {
  Dispatch(new WaylandInput_KeyboardEnter(handle));
}

void WaylandDisplay::KeyboardLeave(unsigned handle) {
  Dispatch(new WaylandInput_KeyboardLeave(handle));
}

// static
void WaylandDisplay::DisplayHandleGlobal(void *data,
    struct wl_registry *registry,
    uint32_t name,
    const char *interface,
    uint32_t version) {

  WaylandDisplay* disp = static_cast<WaylandDisplay*>(data);

  if (strcmp(interface, "wl_compositor") == 0) {
    disp->compositor_ = static_cast<wl_compositor*>(
        wl_registry_bind(registry, name, &wl_compositor_interface, 1));
#if defined(USE_DATA_DEVICE_MANAGER)
  } else if (strcmp(interface, "wl_data_device_manager") == 0) {
    disp->data_device_manager_ = static_cast<wl_data_device_manager*>(
        wl_registry_bind(registry, name, &wl_data_device_manager_interface, 1));
#endif
#if defined(ENABLE_DRM_SUPPORT)
  } else if (!strcmp(interface, "wl_drm")) {
    m_drm = static_cast<struct wl_drm*>(wl_registry_bind(registry,
                                                         name,
                                                         &wl_drm_interface,
                                                         1));
    wl_drm_add_listener(m_drm, &drm_listener, disp);
#endif
  } else if (strcmp(interface, "wl_output") == 0) {
    WaylandScreen* screen = new WaylandScreen(disp->registry(), name);
    if (!disp->screen_list_.empty())
      NOTIMPLEMENTED() << "Multiple screens support is not implemented";

    disp->screen_list_.push_back(screen);
    // (kalyan) Support extended output.
    disp->primary_screen_ = disp->screen_list_.front();
  } else if (strcmp(interface, "wl_seat") == 0) {
    // TODO(mcatanzaro): The display passed to WaylandInputDevice must have a
    // valid data device manager. We should ideally be robust to the compositor
    // advertising a wl_seat first. No known compositor does this, fortunately.
    WaylandSeat* seat = new WaylandSeat(disp, name);
    disp->seat_list_.push_back(seat);
    disp->primary_seat_ = disp->seat_list_.front();
  } else if (strcmp(interface, "wl_shm") == 0) {
    disp->shm_ = static_cast<wl_shm*>(
        wl_registry_bind(registry, name, &wl_shm_interface, 1));
  }
#if defined(OS_WEBOS)
  else if (strcmp(interface, "text_model_factory") == 0) {
    disp->text_model_factory_ = static_cast<text_model_factory*>(
        wl_registry_bind(registry, name, &text_model_factory_interface, 1));
  } else if (strcmp(interface, "wl_webos_input_manager") == 0) {
    disp->webos_input_manager_ = static_cast<wl_webos_input_manager*>(
        wl_registry_bind(registry, name, &wl_webos_input_manager_interface, 1));

    static const struct wl_webos_input_manager_listener
        kWebosInputManagerListener = {
            WaylandDisplay::OnWebosInputPointerListener,
        };

    wl_webos_input_manager_add_listener(disp->webos_input_manager_,
                                        &kWebosInputManagerListener, disp);
  } else if (strcmp(interface, "wl_webos_xinput_extension") == 0) {
    disp->webos_xinput_extension_ =
        static_cast<wl_webos_xinput_extension*>(wl_registry_bind(
            registry, name, &wl_webos_xinput_extension_interface, 1));
    disp->webos_xinput_ =
        wl_webos_xinput_extension_register_input(disp->webos_xinput_extension_);
  } else if (strcmp(interface, "wl_webos_surface_group_compositor") == 0) {
    disp->group_compositor_.reset(
        new WebOSSurfaceGroupCompositor(registry, name));
  }
#else
  else if (strcmp(interface, "wl_text_input_manager") == 0) {
    disp->text_input_manager_ = static_cast<wl_text_input_manager*>(
        wl_registry_bind(registry, name, &wl_text_input_manager_interface, 1));
  }
#endif
  else {
    disp->shell_->Initialize(registry, name, interface, version);
  }
}

void WaylandDisplay::OnChannelEstablished(IPC::Sender* sender) {
  loop_ = base::MessageLoop::current();
  sender_ = sender;
  while (!deferred_messages_.empty()) {
    Dispatch(deferred_messages_.front());
    deferred_messages_.pop();
  }
}

bool WaylandDisplay::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WaylandDisplay, message)
  IPC_MESSAGE_HANDLER(WaylandDisplay_State, SetWidgetState)
  IPC_MESSAGE_HANDLER(WaylandDisplay_Create, CreateWidget)
  IPC_MESSAGE_HANDLER(WaylandDisplay_InitWindow, InitWindow)
  IPC_MESSAGE_HANDLER(WaylandDisplay_DestroyWindow, DestroyWindow)
  IPC_MESSAGE_HANDLER(WaylandDisplay_MoveWindow, MoveWindow)
  IPC_MESSAGE_HANDLER(WaylandDisplay_Title, SetWidgetTitle)
  IPC_MESSAGE_HANDLER(WaylandDisplay_AddRegion, AddRegion)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SubRegion, SubRegion)
  IPC_MESSAGE_HANDLER(WaylandDisplay_CursorSet, SetCursorBitmap)
  IPC_MESSAGE_HANDLER(WaylandDisplay_MoveCursor, MoveCursor)
  IPC_MESSAGE_HANDLER(WaylandDisplay_ImeReset, ResetIme)
  IPC_MESSAGE_HANDLER(WaylandDisplay_ShowInputPanel, ShowInputPanel)
  IPC_MESSAGE_HANDLER(WaylandDisplay_HideInputPanel, HideInputPanel)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetInputContentType, SetInputContentType)
  IPC_MESSAGE_HANDLER(WaylandDisplay_RequestDragData, RequestDragData)
  IPC_MESSAGE_HANDLER(WaylandDisplay_RequestSelectionData, RequestSelectionData)
  IPC_MESSAGE_HANDLER(WaylandDisplay_DragWillBeAccepted, DragWillBeAccepted)
  IPC_MESSAGE_HANDLER(WaylandDisplay_DragWillBeRejected, DragWillBeRejected)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetWindowProperty, SetWindowProperty)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetSurroundingText, SetSurroundingText)
  IPC_MESSAGE_HANDLER(WaylandDisplay_XInputActivate, XInputActivate)
  IPC_MESSAGE_HANDLER(WaylandDisplay_XInputInvokeAction, XInputInvokeAction)
  IPC_MESSAGE_HANDLER(WaylandDisplay_XInputDeactivate, XInputDeactivate)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetInputRegion, SetInputRegion)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetGroupKeyMask, SetGroupKeyMask)
  IPC_MESSAGE_HANDLER(WaylandDisplay_SetKeyMask, SetKeyMask)
  IPC_MESSAGE_HANDLER(WaylandDisplay_CreateWindowGroup, CreateWindowGroup)
  IPC_MESSAGE_HANDLER(WaylandDisplay_AttachToWindowGroup, AttachToWindowGroup)
  IPC_MESSAGE_HANDLER(WaylandDisplay_FocusWindowGroupOwner,
                      FocusWindowGroupOwner)
  IPC_MESSAGE_HANDLER(WaylandDisplay_FocusWindowGroupLayer,
                      FocusWindowGroupLayer)
  IPC_MESSAGE_HANDLER(WaylandDisplay_DetachWindowGroup, DetachWindowGroup)

  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()

  return handled;
}

IPC::MessageFilter* WaylandDisplay::GetMessageFilter() {
  return NULL;
}

void WaylandDisplay::MotionNotify(float x, float y) {
  Dispatch(new WaylandInput_MotionNotify(x, y));
}

void WaylandDisplay::ButtonNotify(unsigned handle,
                                  ui::EventType type,
                                  ui::EventFlags flags,
                                  float x,
                                  float y) {
  Dispatch(new WaylandInput_ButtonNotify(handle, type, flags, x, y));
}

void WaylandDisplay::AxisNotify(float x,
                                float y,
                                int xoffset,
                                int yoffset) {
  Dispatch(new WaylandInput_AxisNotify(x, y, xoffset, yoffset));
}

void WaylandDisplay::PointerEnter(unsigned handle, float x, float y) {
  Dispatch(new WaylandInput_PointerEnter(handle, x, y));
}

void WaylandDisplay::PointerLeave(unsigned handle, float x, float y) {
  Dispatch(new WaylandInput_PointerLeave(handle, x, y));
}

void WaylandDisplay::KeyNotify(ui::EventType type,
                               unsigned code,
                               int device_id) {
  Dispatch(new WaylandInput_KeyNotify(type, code, device_id));
}

void WaylandDisplay::VirtualKeyNotify(ui::EventType type,
                                      uint32_t key,
                                      int device_id) {
  Dispatch(new WaylandInput_VirtualKeyNotify(type, key, device_id));
}

void WaylandDisplay::TouchNotify(ui::EventType type,
                                 float x,
                                 float y,
                                 int32_t touch_id,
                                 uint32_t time_stamp) {
  Dispatch(new WaylandInput_TouchNotify(type, x, y, touch_id, time_stamp));
}

void WaylandDisplay::OutputScreenChanged(unsigned width,
                                         unsigned height,
                                         int rotation) {
  Dispatch(new WaylandOutput_ScreenChanged(width, height, rotation));
}

void WaylandDisplay::WindowResized(unsigned handle,
                                   unsigned width,
                                   unsigned height) {
  Dispatch(new WaylandWindow_Resized(handle, width, height));
}

void WaylandDisplay::WindowUnminimized(unsigned handle) {
  Dispatch(new WaylandWindow_Unminimized(handle));
}

void WaylandDisplay::WindowDeActivated(unsigned windowhandle) {
  Dispatch(new WaylandWindow_DeActivated(windowhandle));
}

void WaylandDisplay::WindowActivated(unsigned windowhandle) {
  Dispatch(new WaylandWindow_Activated(windowhandle));
}

void WaylandDisplay::CloseWidget(unsigned handle) {
  Dispatch(new WaylandInput_CloseWidget(handle));
}

void WaylandDisplay::Commit(unsigned handle,
                            const std::string& text) {
  Dispatch(new WaylandInput_Commit(handle, text));
}

void WaylandDisplay::PreeditChanged(unsigned handle,
                                    const std::string& text,
                                    const std::string& commit) {
  Dispatch(new WaylandInput_PreeditChanged(handle, text, commit));
}

void WaylandDisplay::DeleteRange(unsigned handle,
                                 int32_t index,
                                 uint32_t length) {
  Dispatch(new WaylandInput_DeleteRange(handle, index, length));
}

void WaylandDisplay::PreeditEnd() {
  Dispatch(new WaylandInput_PreeditEnd());
}

void WaylandDisplay::PreeditStart() {
  Dispatch(new WaylandInput_PreeditStart());
}

void WaylandDisplay::InitializeXKB(base::SharedMemoryHandle fd, uint32_t size) {
  Dispatch(new WaylandInput_InitializeXKB(fd, size));
}

void WaylandDisplay::DragEnter(unsigned windowhandle,
                               float x,
                               float y,
                               const std::vector<std::string>& mime_types,
                               uint32_t serial) {
  Dispatch(new WaylandInput_DragEnter(windowhandle, x, y, mime_types, serial));
}

void WaylandDisplay::DragData(unsigned windowhandle,
                              base::FileDescriptor pipefd) {
  Dispatch(new WaylandInput_DragData(windowhandle, pipefd));
}

void WaylandDisplay::DragLeave(unsigned windowhandle) {
  Dispatch(new WaylandInput_DragLeave(windowhandle));
}

void WaylandDisplay::DragMotion(unsigned windowhandle,
                                float x,
                                float y,
                                uint32_t time) {
  Dispatch(new WaylandInput_DragMotion(windowhandle, x, y, time));
}

void WaylandDisplay::DragDrop(unsigned windowhandle) {
  Dispatch(new WaylandInput_DragDrop(windowhandle));
}

void WaylandDisplay::Dispatch(IPC::Message* message) {
  if (!loop_) {
    deferred_messages_.push(message);
    return;
  }

  loop_->task_runner()->PostTask(FROM_HERE,
      base::Bind(&WaylandDisplay::Send,
                 weak_ptr_factory_.GetWeakPtr(),
                 message));
}

void WaylandDisplay::Send(IPC::Message* message) {
  // The GPU process never sends synchronous IPC, so clear the unblock flag.
  // This ensures the message is treated as a synchronous one and helps preserve
  // order. Check set_unblock in ipc_messages.h for explanation.
  message->set_unblock(true);
  sender_->Send(message);
}

}  // namespace ozonewayland

