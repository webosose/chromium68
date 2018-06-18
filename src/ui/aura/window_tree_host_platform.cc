// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/aura/window_tree_host_platform.h"

#include <utility>

#include "base/command_line.h"
#include "base/macros.h"
#include "base/run_loop.h"
#include "base/trace_event/trace_event.h"
#include "build/build_config.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/aura/window_port.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/layout.h"
#include "ui/base/ui_base_neva_switches.h"
#include "ui/compositor/compositor.h"
#include "ui/events/event.h"
#include "ui/events/keyboard_hook.h"
#include "ui/events/keycodes/dom/dom_code.h"
#include "ui/platform_window/platform_window_init_properties.h"

#if defined(OS_ANDROID)
#include "ui/platform_window/android/platform_window_android.h"
#endif

#if defined(USE_OZONE)
#include "ui/ozone/public/ozone_platform.h"
#endif

#if defined(OS_WIN)
#include "ui/base/cursor/cursor_loader_win.h"
#include "ui/platform_window/win/win_window.h"
#endif

#if defined(USE_X11)
#include "ui/platform_window/x11/x11_window.h"
#endif

namespace aura {

// static
WindowTreeHost* WindowTreeHost::Create(const gfx::Rect& bounds) {
  return new WindowTreeHostPlatform(bounds);
}

WindowTreeHostPlatform::WindowTreeHostPlatform(const gfx::Rect& bounds)
    : WindowTreeHostPlatform() {
  bounds_ = bounds;
  CreateCompositor();

  ui::PlatformWindowInitProperties properties;
  properties.bounds = bounds_;
  CreateAndSetPlatformWindow(std::move(properties));
}

WindowTreeHostPlatform::WindowTreeHostPlatform()
    : WindowTreeHostPlatform(nullptr) {}

WindowTreeHostPlatform::WindowTreeHostPlatform(
    std::unique_ptr<WindowPort> window_port)
    : WindowTreeHost(std::move(window_port)),
      widget_(gfx::kNullAcceleratedWidget),
      current_cursor_(ui::CursorType::kNull) {}

void WindowTreeHostPlatform::CreateAndSetPlatformWindow(
    ui::PlatformWindowInitProperties properties) {
#if defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  platform_window_ =
      ui::OzonePlatform::GetInstance()->CreatePlatformWindow(this, properties.bounds);
  bool ime_enabled =
      base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableNevaIme);
  if (ime_enabled)
    GetInputMethod()->AddObserver(this);
  SetImeEnabled(ime_enabled);
#elif defined(USE_OZONE)
  platform_window_ =
      ui::OzonePlatform::GetInstance()->CreatePlatformWindow(this, std::move(properties)); 
#elif defined(OS_WIN)
  platform_window_.reset(new ui::WinWindow(this, properties.bounds));
#elif defined(OS_ANDROID)
  platform_window_.reset(new ui::PlatformWindowAndroid(this));
#elif defined(USE_X11)
  platform_window_.reset(new ui::X11Window(this, properties.bounds));
#else
  NOTIMPLEMENTED();
#endif
}

void WindowTreeHostPlatform::SetPlatformWindow(
    std::unique_ptr<ui::PlatformWindow> window) {
  platform_window_ = std::move(window);
}

WindowTreeHostPlatform::~WindowTreeHostPlatform() {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kEnableNevaIme))
    GetInputMethod()->RemoveObserver(this);
  DestroyCompositor();
  DestroyDispatcher();

  // |platform_window_| might have already been destroyed by this time.
  if (platform_window_)
    platform_window_->Close();
}

ui::EventSource* WindowTreeHostPlatform::GetEventSource() {
  return this;
}

gfx::AcceleratedWidget WindowTreeHostPlatform::GetAcceleratedWidget() {
  return widget_;
}

void WindowTreeHostPlatform::ShowImpl() {
  platform_window_->Show();
}

void WindowTreeHostPlatform::HideImpl() {
  platform_window_->Hide();
}

gfx::Rect WindowTreeHostPlatform::GetBoundsInPixels() const {
  return platform_window_ ? platform_window_->GetBounds() : gfx::Rect();
}

void WindowTreeHostPlatform::SetBoundsInPixels(
    const gfx::Rect& bounds,
    const viz::LocalSurfaceId& local_surface_id) {
  pending_size_ = bounds.size();
  pending_local_surface_id_ = local_surface_id;
  platform_window_->SetBounds(bounds);
}

gfx::Point WindowTreeHostPlatform::GetLocationOnScreenInPixels() const {
  return platform_window_->GetBounds().origin();
}

void WindowTreeHostPlatform::SetCapture() {
  platform_window_->SetCapture();
}

void WindowTreeHostPlatform::ReleaseCapture() {
  platform_window_->ReleaseCapture();
}

void WindowTreeHostPlatform::SetWindowProperty(const std::string& name,
                                               const std::string& value) {
#if defined(USE_OZONE)
  platform_window_->SetWindowProperty(name, value);
#endif
}

bool WindowTreeHostPlatform::CaptureSystemKeyEventsImpl(
    base::Optional<base::flat_set<ui::DomCode>> dom_codes) {
  // Only one KeyboardHook should be active at a time, otherwise there will be
  // problems with event routing (i.e. which Hook takes precedence) and
  // destruction ordering.
  DCHECK(!keyboard_hook_);
  keyboard_hook_ = ui::KeyboardHook::Create(
      std::move(dom_codes), GetAcceleratedWidget(),
      base::BindRepeating(
          [](ui::PlatformWindowDelegate* delegate, ui::KeyEvent* event) {
            delegate->DispatchEvent(event);
          },
          base::Unretained(this)));

  return keyboard_hook_ != nullptr;
}

void WindowTreeHostPlatform::ReleaseSystemKeyEventCapture() {
  keyboard_hook_.reset();
}

bool WindowTreeHostPlatform::IsKeyLocked(ui::DomCode dom_code) {
  return keyboard_hook_ && keyboard_hook_->IsKeyLocked(dom_code);
}

base::flat_map<std::string, std::string>
WindowTreeHostPlatform::GetKeyboardLayoutMap() {
  NOTIMPLEMENTED();
  return {};
}

void WindowTreeHostPlatform::SetWindowSurfaceId(int surface_id) {
  platform_window_->SetSurfaceId(surface_id);
}

void WindowTreeHostPlatform::SetCursorNative(gfx::NativeCursor cursor) {
  if (cursor == current_cursor_)
    return;
  current_cursor_ = cursor;

#if defined(OS_WIN)
  ui::CursorLoaderWin cursor_loader;
  cursor_loader.SetPlatformCursor(&cursor);
#endif

  // Setting custom cursor from a higher layer (Chromium) is disabled in WebOS.
  // Thus, to provide backward compatibility, another API needs to be implemen-
  // ted to preserve capability of changing the default cursor appearance to a
  // custom one by demand (not from the upstream layer) instead.
#if defined(OS_WEBOS)
  return;
#endif

  platform_window_->SetCursor(cursor.platform());
}

void WindowTreeHostPlatform::MoveCursorToScreenLocationInPixels(
    const gfx::Point& location_in_pixels) {
  platform_window_->MoveCursorTo(location_in_pixels);
}

void WindowTreeHostPlatform::OnCursorVisibilityChangedNative(bool show) {
  NOTIMPLEMENTED();
}

void WindowTreeHostPlatform::OnBoundsChanged(const gfx::Rect& new_bounds) {
  float current_scale = compositor()->device_scale_factor();
  float new_scale = ui::GetScaleFactorForNativeView(window());
  gfx::Rect old_bounds = bounds_;
  bounds_ = new_bounds;
  if (bounds_.origin() != old_bounds.origin())
    OnHostMovedInPixels(bounds_.origin());
  if (pending_local_surface_id_.is_valid() ||
      bounds_.size() != old_bounds.size() || current_scale != new_scale) {
    auto local_surface_id = bounds_.size() == pending_size_
                                ? pending_local_surface_id_
                                : viz::LocalSurfaceId();
    pending_local_surface_id_ = viz::LocalSurfaceId();
    pending_size_ = gfx::Size();
    OnHostResizedInPixels(bounds_.size(), local_surface_id);
  }
}

void WindowTreeHostPlatform::OnDamageRect(const gfx::Rect& damage_rect) {
  compositor()->ScheduleRedrawRect(damage_rect);
}

void WindowTreeHostPlatform::DispatchEvent(ui::Event* event) {
  TRACE_EVENT0("input", "WindowTreeHostPlatform::DispatchEvent");
  ui::EventDispatchDetails details = SendEventToSink(event);
  if (details.dispatcher_destroyed)
    event->SetHandled();
}

void WindowTreeHostPlatform::OnCloseRequest() {
#if defined(OS_WIN)
  // TODO: this obviously shouldn't be here.
  base::RunLoop::QuitCurrentWhenIdleDeprecated();
#else
  OnHostCloseRequested();
#endif
}

void WindowTreeHostPlatform::OnClosed() {}

void WindowTreeHostPlatform::OnWindowStateChanged(
    ui::PlatformWindowState new_state) {}

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
void WindowTreeHostPlatform::OnWindowHostStateChanged(
    ui::WidgetState new_state) {
  WindowTreeHost::OnWindowHostStateChanged(new_state);
}
#endif

void WindowTreeHostPlatform::OnLostCapture() {
  OnHostLostWindowCapture();
}

void WindowTreeHostPlatform::OnAcceleratedWidgetAvailable(
    gfx::AcceleratedWidget widget,
    float device_pixel_ratio) {
  widget_ = widget;
  // This may be called before the Compositor has been created.
  if (compositor())
    WindowTreeHost::OnAcceleratedWidgetAvailable();
}

void WindowTreeHostPlatform::OnAcceleratedWidgetDestroying() {}

void WindowTreeHostPlatform::OnAcceleratedWidgetDestroyed() {
  gfx::AcceleratedWidget widget = compositor()->ReleaseAcceleratedWidget();
  DCHECK_EQ(widget, widget_);
  widget_ = gfx::kNullAcceleratedWidget;
}

void WindowTreeHostPlatform::OnActivationChanged(bool active) {
  if (active)
    OnHostActivated();
}

void WindowTreeHostPlatform::OnShowIme() {
#if defined(USE_OZONE)
  platform_window_->ShowInputPanel();
#endif
}

void WindowTreeHostPlatform::OnHideIme() {
#if defined(USE_OZONE)
  platform_window_->HideInputPanel();
#endif
}

void WindowTreeHostPlatform::OnTextInputTypeChanged(ui::TextInputType text_input_type,
                                                    int text_input_flags) {
#if defined(USE_OZONE)
  if (text_input_type != ui::TEXT_INPUT_TYPE_NONE)
    platform_window_->SetInputContentType(text_input_type, text_input_flags);
#endif
}

///@name USE_NEVA_APPRUNTIME
///@{
void WindowTreeHostPlatform::SetSurroundingText(const std::string& text,
                                                size_t cursor_position,
                                                size_t anchor_position) {
#if defined(USE_OZONE)
  platform_window_->SetSurroundingText(text, cursor_position, anchor_position);
#endif
}
///@}

}  // namespace aura
