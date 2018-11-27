// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include "neva/app_runtime/webapp_window.h"

#include <climits>
#include <cstdint>

#include "base/strings/utf_string_conversions.h"
#include "content/browser/web_contents/web_contents_view.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/web_contents.h"
#include "neva/app_runtime/public/app_runtime_event.h"
#include "neva/app_runtime/public/webapp_window_delegate.h"
#include "neva/app_runtime/public/window_group_configuration.h"
#include "neva/app_runtime/ui/desktop_aura/app_runtime_desktop_native_widget_aura.h"
#include "ui/aura/window.h"
#include "ui/aura/window_tree_host.h"
#include "ui/base/ime/input_method.h"
#include "ui/base/ime/text_input_client.h"
#include "ui/base/ui_base_types.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/gfx/geometry/size.h"
#include "ui/platform_window/neva/window_group_configuration.h"
#include "ui/platform_window/neva/xinput_types.h"
#include "ui/views/controls/webview/webview.h"
#include "ui/views/layout/fill_layout.h"
#include "ui/views/view.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"
#include "ui/views/test/desktop_test_views_delegate.h"

namespace app_runtime {

namespace {

const std::size_t kUInt32BitLength = sizeof(std::uint32_t) * CHAR_BIT;

const int kKeyboardHeightMargin = 10;
const int kKeyboardAnimationTime = 600;

using WidgetType = app_runtime::WebAppWindowBase::CreateParams::WidgetType;
using WindowShowState =
    app_runtime::WebAppWindowBase::CreateParams::WindowShowState;

views::Widget::InitParams::Type ToViewsWigetType(WidgetType type) {
  switch (type) {
    case WidgetType::kWindow :
      return views::Widget::InitParams::TYPE_WINDOW;
    case WidgetType::kPanel :
      return views::Widget::InitParams::TYPE_PANEL;
    case WidgetType::kWindowFrameless :
      return views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
    case WidgetType::kControl :
      return views::Widget::InitParams::TYPE_CONTROL;
    case WidgetType::kPopup :
      return views::Widget::InitParams::TYPE_POPUP;
    case WidgetType::kMenu :
      return views::Widget::InitParams::TYPE_MENU;
    case WidgetType::kTooltip :
      return views::Widget::InitParams::TYPE_TOOLTIP;
    case WidgetType::kBubble :
      return views::Widget::InitParams::TYPE_BUBBLE;
    case WidgetType::kDrag :
      return views::Widget::InitParams::TYPE_DRAG;
  }

  NOTREACHED();
}

ui::WindowShowState ToUiWindowShowState(WindowShowState state) {
  switch (state) {
    case WindowShowState::kDefault :
      return ui::SHOW_STATE_DEFAULT;
    case WindowShowState::kNormal :
      return ui::SHOW_STATE_NORMAL;
    case WindowShowState::kMinimized :
      return ui::SHOW_STATE_MINIMIZED;
    case WindowShowState::kMaximized :
      return ui::SHOW_STATE_MAXIMIZED;
    case WindowShowState::kInactive :
      return ui::SHOW_STATE_INACTIVE;
    case WindowShowState::kFullscreen :
      return ui::SHOW_STATE_FULLSCREEN;
    case WindowShowState::kEnd :
      return ui::SHOW_STATE_END;
  }

  NOTREACHED();
}

inline bool ExistsInUiKeyMaskType(std::uint32_t key_mask) {
  ui::KeyMask key_masks = static_cast<ui::KeyMask>(key_mask);
  switch (key_masks) {
    case ui::KeyMask::kHome:
    case ui::KeyMask::kBack:
    case ui::KeyMask::kExit:
    case ui::KeyMask::kNavigationLeft:
    case ui::KeyMask::kNavigationRight:
    case ui::KeyMask::kNavigationUp:
    case ui::KeyMask::kNavigationDown:
    case ui::KeyMask::kNavigationOk:
    case ui::KeyMask::kNumericKeys:
    case ui::KeyMask::kRemoteColorRed:
    case ui::KeyMask::kRemoteColorGreen:
    case ui::KeyMask::kRemoteColorYellow:
    case ui::KeyMask::kRemoteColorBlue:
    case ui::KeyMask::kRemoteProgrammeGroup:
    case ui::KeyMask::kRemotePlaybackGroup:
    case ui::KeyMask::kRemoteTeletextGroup:
    case ui::KeyMask::kLocalLeft:
    case ui::KeyMask::kLocalRight:
    case ui::KeyMask::kLocalUp:
    case ui::KeyMask::kLocalDown:
    case ui::KeyMask::kLocalOk:
    case ui::KeyMask::kRemoteMagnifierGroup:
    case ui::KeyMask::kMinimalPlaybackGroup:
      return true;
    default:
      LOG(WARNING) << __func__ << "(): unknown key mask value: " << key_mask;
      return false;
  }
}

}  // namespace

WebAppWindow::WebAppWindow(const WebAppWindowBase::CreateParams& params,
                           WebAppWindowDelegate* delegate)
    : delegate_(delegate),
      params_(params),
      rect_(gfx::Size(params.width, params.height)),
      window_host_state_(ui::WidgetState::UNINITIALIZED),
      current_rotation_(-1),
      window_host_state_about_to_change_(ui::WidgetState::UNINITIALIZED) {
  InitWindow();

  ComputeScaleFactor();

  if (params.web_contents) {
    SetupWebContents(static_cast<content::WebContents*>(params.web_contents));
  }

  if (display::Screen::GetScreen()->GetNumDisplays() > 0) {
    current_rotation_ =
        display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();
    ComputeScaleFactor();
  }

  display::Screen::GetScreen()->AddObserver(this);
}

WebAppWindow::~WebAppWindow() {
  display::Screen::GetScreen()->RemoveObserver(this);
  if (desktop_native_widget_aura_) {
    desktop_native_widget_aura_->OnWebAppWindowRemoved();
    desktop_native_widget_aura_->SetNativeEventDelegate(nullptr);
  }
}

void WebAppWindow::OnDisplayAdded(const display::Display& new_display) {
  current_rotation_ =
      display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();
  ComputeScaleFactor();
}

void WebAppWindow::OnDisplayRemoved(const display::Display& old_display) {}

void WebAppWindow::OnDisplayMetricsChanged(const display::Display& display,
                                           uint32_t metrics) {
  if (metrics & display::DisplayObserver::DISPLAY_METRIC_ROTATION) {
    // Get new rotation from primary display
    int screen_rotation =
        display::Screen::GetScreen()->GetPrimaryDisplay().RotationAsDegree();

    if (screen_rotation == current_rotation_)
      return;

    // FIXME: Wayland shell surface already reports resize request but for now
    // LSM fails to report that resize is due to rotation change. We need this
    // information in order do swap width and height because otherwise by
    // default resizing window is denied by allow_window_resize_ flag. If LSM
    // would report it correctly then this width and height swapping would not
    // need to be done here but on ozone wayland level.
    int new_width = rect_.width();
    int new_height = rect_.height();
    if (rect_.width() != 0 && rect_.height() != 0) {
      if (current_rotation_ == 0 || current_rotation_ == 180) {
        if (screen_rotation == 90 || screen_rotation == 270) {
          new_width = rect_.height();
          new_height = rect_.width();
        }
      } else if (current_rotation_ == 90 || current_rotation_ == 270) {
        if (screen_rotation == 0 || screen_rotation == 180) {
          new_width = rect_.height();
          new_height = rect_.width();
        }
      }
    }

    current_rotation_ = screen_rotation;

    if (new_width != rect_.width() || new_height != rect_.height())
      Resize(new_width, new_height);

    ComputeScaleFactor();
  }
}

bool WebAppWindow::Event(AppRuntimeEvent* app_runtime_event) {
  if (delegate_)
    return delegate_->event(app_runtime_event);

  return false;
}

void WebAppWindow::OnDesktopNativeWidgetAuraRemoved() {
  desktop_native_widget_aura_ = nullptr;
}

bool WebAppWindow::IsInputPanelVisible() const {
  return input_panel_visible_;
}

void WebAppWindow::SetUseVirtualKeyboard(bool enable) {
  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetUseVirtualKeyboard(enable);
}

void WebAppWindow::SetupWebContents(content::WebContents* web_contents) {
  if (!webview_) {
    webview_ = new views::WebView(web_contents->GetBrowserContext());
    SetLayoutManager(std::make_unique<views::FillLayout>());
    AddChildView(webview_);
    Layout();
  }

  webview_->SetWebContents(web_contents);
  web_contents_ = web_contents;
}

content::WebContents* WebAppWindow::GetWebContents() {
  return web_contents_;
}

// static
WidgetState WebAppWindow::ToExposedWidgetStateType(ui::WidgetState state) {
  switch (state) {
    case ui::WidgetState::SHOW:
      return WidgetState::SHOW;
    case ui::WidgetState::HIDE:
      return WidgetState::HIDE;
    case ui::WidgetState::FULLSCREEN:
      return WidgetState::FULLSCREEN;
    case ui::WidgetState::MAXIMIZED:
      return WidgetState::MAXIMIZED;
    case ui::WidgetState::MINIMIZED:
      return WidgetState::MINIMIZED;
    case ui::WidgetState::RESTORE:
      return WidgetState::RESTORE;
    case ui::WidgetState::ACTIVE:
      return WidgetState::ACTIVE;
    case ui::WidgetState::INACTIVE:
      return WidgetState::INACTIVE;
    case ui::WidgetState::RESIZE:
      return WidgetState::RESIZE;
    case ui::WidgetState::DESTROYED:
      return WidgetState::DESTROYED;
    default:
      return WidgetState::UNINITIALIZED;
  }
}

// static
ui::WidgetState WebAppWindow::FromExposedWidgetStateType(WidgetState state) {
  switch (state) {
    case WidgetState::SHOW:
      return ui::WidgetState::SHOW;
    case WidgetState::HIDE:
      return ui::WidgetState::HIDE;
    case WidgetState::FULLSCREEN:
      return ui::WidgetState::FULLSCREEN;
    case WidgetState::MAXIMIZED:
      return ui::WidgetState::MAXIMIZED;
    case WidgetState::MINIMIZED:
      return ui::WidgetState::MINIMIZED;
    case WidgetState::RESTORE:
      return ui::WidgetState::RESTORE;
    case WidgetState::ACTIVE:
      return ui::WidgetState::ACTIVE;
    case WidgetState::INACTIVE:
      return ui::WidgetState::INACTIVE;
    case WidgetState::RESIZE:
      return ui::WidgetState::RESIZE;
    case WidgetState::DESTROYED:
      return ui::WidgetState::DESTROYED;
    default:
      return ui::WidgetState::UNINITIALIZED;
  }
}

void WebAppWindow::SetWindowHostState(ui::WidgetState state) {
  if (!host_)
    return;

  switch (state) {
  case ui::WidgetState::FULLSCREEN:
    host_->SetFullscreen(true);
    break;
  case ui::WidgetState::MAXIMIZED:
    if (host_->IsFullscreen())
      host_->SetFullscreen(false);

    host_->Maximize();
    break;
  case ui::WidgetState::MINIMIZED:
    host_->Minimize();
    break;
  default:
    break;
  }
}

ui::WidgetState WebAppWindow::GetWindowHostState() const {
  return window_host_state_;
}

ui::WidgetState WebAppWindow::GetWindowHostStateAboutToChange() const {
  return window_host_state_about_to_change_;
}

void WebAppWindow::Activate() {
  if (widget_)
    widget_->Activate();
}

void WebAppWindow::SetBounds(int x, int y, int width, int height) {
  if (widget_)
    widget_->SetBounds(gfx::Rect(x, y, width, height));
}

void WebAppWindow::SetWindowProperty(const std::string& name,
                                     const std::string& value) {
  window_property_list_[name] = value;

  if (!host_)
    return;

  LOG(INFO) << __func__
            << "(): SetWindowProperty is called. Name: " << name.c_str()
            << " Value: " << value.c_str();

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetWindowProperty(name, value);
}

void WebAppWindow::Show() {
  widget_->Show();
}

void WebAppWindow::Hide() {
  if (widget_)
    widget_->Hide();
}

void WebAppWindow::Minimize() {
  if (widget_)
    widget_->Minimize();
}

void WebAppWindow::Restore() {
  if (widget_)
    widget_->Restore();
}

void WebAppWindow::Close() {
  widget_closed_ = true;

  if (widget_)
    widget_->Close();
  else if (deferred_deleting_)
    delete this;
}

void WebAppWindow::CompositorBuffersSwapped() {
}

void WebAppWindow::CursorVisibilityChange(bool visible) {
}

bool WebAppWindow::IsTextInputOverlapped() {
  ui::InputMethod* ime = host_->AsWindowTreeHost()->GetInputMethod();
  gfx::Rect caret_bounds = ime->GetTextInputClient()->GetCaretBounds();
  int caret_bottom = caret_bounds.y() + caret_bounds.height() -
                     web_contents_->GetContentNativeView()->bounds().y();
  int viewport_height = rect_.height();
  return viewport_height - input_panel_height() <
         caret_bottom + kKeyboardHeightMargin;
}

void WebAppWindow::InputPanelVisibilityChanged(bool visible) {
  if (visible) {
    if (input_panel_rect_.height() && IsTextInputOverlapped()) {
      viewport_shift_height_ = -input_panel_height();
      UpdateViewportY();
    }
  } else {
    viewport_shift_height_ = 0;
    UpdateViewportY();
  }

  input_panel_visible_ = visible;
}

void WebAppWindow::InputPanelRectChanged(int32_t x,
                                         int32_t y,
                                         uint32_t width,
                                         uint32_t height) {
  input_panel_rect_.SetRect(x, y, width, height);
  if (input_panel_rect_.height() && input_panel_visible_) {
    viewport_shift_height_ =
        IsTextInputOverlapped() ? -input_panel_height() : 0;
    UpdateViewportY();
  }
}

void WebAppWindow::UpdateViewportY() {
  gfx::Rect bounds = web_contents_->GetContentNativeView()->bounds();
  if (bounds.y() == viewport_shift_height_)
    return;

  if (bounds.y()) {
    UpdateViewportYCallback();
  } else {
    if (viewport_timer_.IsRunning())
      viewport_timer_.Reset();
    else
      viewport_timer_.Start(
          FROM_HERE, base::TimeDelta::FromMilliseconds(kKeyboardAnimationTime),
          this, &WebAppWindow::UpdateViewportYCallback);
  }
}

void WebAppWindow::UpdateViewportYCallback() {
  gfx::Rect bounds = web_contents_->GetContentNativeView()->bounds();
  web_contents_->GetContentNativeView()->SetBounds(gfx::Rect(
      bounds.x(), viewport_shift_height_, bounds.width(), bounds.height()));
}

void WebAppWindow::KeyboardEnter() {
}

void WebAppWindow::KeyboardLeave() {
}

void WebAppWindow::WindowHostClose() {
}

void WebAppWindow::WindowHostExposed() {
}

void WebAppWindow::WindowHostStateChanged(ui::WidgetState new_state) {
  if (window_host_state_ == new_state)
    return;

  window_host_state_ = new_state;

  AppRuntimeEvent app_runtime_event(AppRuntimeEvent::Type::WindowStateChange);
  Event(&app_runtime_event);
}

void WebAppWindow::WindowHostStateAboutToChange(ui::WidgetState state) {
  window_host_state_about_to_change_ = state;

  AppRuntimeEvent app_runtime_event(AppRuntimeEvent::Type::WindowStateAboutToChange);
  Event(&app_runtime_event);
}

views::Widget* WebAppWindow::GetWidget() {
  return widget_;
}

const views::Widget* WebAppWindow::GetWidget() const {
  return widget_;
}

bool WebAppWindow::CanResize() const {
  return false;
}

bool WebAppWindow::CanMaximize() const {
  return true;
}

bool WebAppWindow::ShouldShowWindowTitle() const {
  return true;
}

bool WebAppWindow::ShouldShowCloseButton() const {
  return false;
}

views::View* WebAppWindow::GetContentsView() {
  return this;
}

void WebAppWindow::ComputeScaleFactor() {
  int display_height =
      display::Screen::GetScreen()->GetPrimaryDisplay().bounds().height();
  scale_factor_ = static_cast<float>(display_height) / rect_.height();
}

void WebAppWindow::Resize(int width, int height) {
  if (rect_.width() != width || rect_.height() != height) {
    rect_.set_width(width);
    rect_.set_height(height);

    ComputeScaleFactor();

    if (widget_)
      widget_->SetSize(gfx::Size(width, height));
  }
}

void WebAppWindow::SetInputRegion(const std::vector<gfx::Rect>& region) {
  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetInputRegion(region);
}

// static
ui::KeyMask WebAppWindow::FromExposedKeyMaskType(KeyMask key_mask) {
  std::uint32_t key_masks = static_cast<std::uint32_t>(key_mask);
  std::uint32_t result = 0;

  for (std::size_t i = 0; i < kUInt32BitLength; ++i) {
    std::uint32_t mask = 1 << i;
    if ((key_masks & mask) && ExistsInUiKeyMaskType(mask))
      result |= mask;
  }

  return static_cast<ui::KeyMask>(result);
}

void WebAppWindow::SetGroupKeyMask(ui::KeyMask key_mask) {
  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetGroupKeyMask(key_mask);
}

void WebAppWindow::SetKeyMask(ui::KeyMask key_mask, bool set) {
  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetKeyMask(key_mask, set);
}

void WebAppWindow::SetOpacity(float opacity) {
  DCHECK(opacity >= 0.f && opacity <= 1.f) << "invalid opacity value provided";

  if (!host_)
    return;

  host_->SetOpacity(opacity);
}

void WebAppWindow::SetRootLayerOpacity(float opacity) {
  DCHECK(opacity >= 0.f && opacity <= 1.f) << "invalid opacity value provided";

  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetRootLayerOpacity(opacity);
}

void WebAppWindow::SetScaleFactor(float scale) {
  DCHECK(scale > 0.f) << "invalid scale factor value provided";

  if (!host_)
    return;

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->SetScaleFactor(scale);
}

void WebAppWindow::SetCustomCursor(CustomCursorType type,
                                   const std::string& path,
                                   int hotspot_x,
                                   int hotspot_y) {
  auto* wth(host_->AsWindowTreeHost());
  if (!wth)
    return;

  wth->SetCustomCursor(type, path, hotspot_x, hotspot_y);
}

void WebAppWindow::SetWindowTitle(const base::string16& title) {
  title_ = title;
  if (widget_)
    widget_->UpdateWindowTitle();
}

base::string16 WebAppWindow::GetWindowTitle() const {
  return title_;
}

void WebAppWindow::WindowClosing() {
  if (delegate_)
    delegate_->OnWindowClosing();
}

void WebAppWindow::CloseWindow() {
  // Remove a window, but keep a renderer alive.
  if (host_)
    host_->Close();
}

void WebAppWindow::DeleteDelegate() {
  // In case of deferred_deleting_, WebAppWindow will be deleted by owner
  if (!deferred_deleting_ || widget_closed_)
    delete this;
  else {
    if (webview_) {
      delete webview_;
      webview_ = nullptr;
    }

    host_ = nullptr;
    widget_ = nullptr;
  }
}

void WebAppWindow::OnMouseEvent(ui::MouseEvent* event) {
  NOTIMPLEMENTED();
}

void WebAppWindow::OnKeyEvent(ui::KeyEvent* event) {
  switch (event->type()) {
    case ui::EventType::ET_KEY_PRESSED:
      {
        AppRuntimeKeyEvent app_runtime_key_event(
            AppRuntimeEvent::Type::KeyPress, event->key_code());
        Event(&app_runtime_key_event);
      }
      break;
    case ui::EventType::ET_KEY_RELEASED:
      {
        AppRuntimeKeyEvent app_runtime_key_event(
            AppRuntimeEvent::Type::KeyRelease, event->key_code());
        Event(&app_runtime_key_event);
      }
      break;
    default:
      LOG(WARNING) << __func__ << "(): unknown key event type: " <<
          event->type();
      break;
  }
}

void WebAppWindow::InitWindow() {
  widget_ = new views::Widget();
  views::Widget::InitParams init_params(ToViewsWigetType(params_.type));
  init_params.bounds =
      gfx::Rect(params_.pos_x, params_.pos_y, params_.width, params_.height);

  init_params.delegate = this;
  init_params.show_state = ToUiWindowShowState(params_.show_state);
  app_runtime::AppRuntimeDesktopNativeWidgetAura* desktop_native_widget_aura_ =
      new app_runtime::AppRuntimeDesktopNativeWidgetAura(this);
  desktop_native_widget_aura_->SetNativeEventDelegate(this);
  init_params.native_widget = desktop_native_widget_aura_;
  init_params.desktop_window_tree_host = host_ =
      views::DesktopWindowTreeHost::Create(widget_,
                                           desktop_native_widget_aura_);

  if (host_) {
    aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
    DCHECK(wth) << "aura::WindowTreeHost is unavailable";

    wth->AddPreTargetHandler(this);
  }

  widget_->Init(init_params);

  if (params_.show_state == app_runtime::WebAppWindowBase::CreateParams::WindowShowState::kFullscreen)
    widget_->SetFullscreen(true);
}

void WebAppWindow::RecreateIfNeeded() {
  if (widget_ || host_ || webview_ || !GetWebContents())
    return;

  SetupWebContents(GetWebContents());

  InitWindow();

  if (!host_)
    return;

  aura::WindowTreeHost* tree_host = host_->AsWindowTreeHost();
  for (const auto& property : window_property_list_)
    tree_host->SetWindowProperty(property.first, property.second);
}
void WebAppWindow::CompositorResumeDrawing() {
  if (!host_)
    return;

  LOG(INFO) << __func__ << "(): Reset compositor painting";

  aura::WindowTreeHost* wth = host_->AsWindowTreeHost();
  DCHECK(wth) << "aura::WindowTreeHost is unavailable";

  wth->CompositorResumeDrawing();
}

void WebAppWindow::XInputActivate(const std::string& type) {
  if (host_)
    host_->AsWindowTreeHost()->XInputActivate(type);
}

void WebAppWindow::XInputDeactivate() {
  if (host_)
    host_->AsWindowTreeHost()->XInputDeactivate();
}

void WebAppWindow::XInputInvokeAction(uint32_t keysym,
                                      XInputKeySymbolType symbol_type,
                                      XInputEventType event_type) {
  if (!host_)
    return;

  ui::XInputKeySymbolType ui_symbol_type;
  switch (symbol_type) {
    case XINPUT_QT_KEY_SYMBOL:
      ui_symbol_type = ui::XINPUT_QT_KEY_SYMBOL;
      break;
    case XINPUT_NATIVE_KEY_SYMBOL:
      ui_symbol_type = ui::XINPUT_NATIVE_KEY_SYMBOL;
      break;
  };

  ui::XInputEventType ui_event_type;
  switch (event_type) {
    case XINPUT_PRESS_AND_RELEASE:
      ui_event_type = ui::XINPUT_PRESS_AND_RELEASE;
      break;
    case XINPUT_PRESS:
      ui_event_type = ui::XINPUT_PRESS;
      break;
    case XINPUT_RELEASE:
      ui_event_type = ui::XINPUT_RELEASE;
      break;
  };

  host_->AsWindowTreeHost()->XInputInvokeAction(keysym, ui_symbol_type,
                                                ui_event_type);
}

int WebAppWindow::GetWidth() const {
  return widget_ ? widget_->GetWindowBoundsInScreen().width() : 0;
}

int WebAppWindow::GetHeight() const {
  return widget_ ? widget_->GetWindowBoundsInScreen().height() : 0;
}

void WebAppWindow::CreateGroup(
    const app_runtime::WindowGroupConfiguration& config) {
  if (!host_)
    return;

  ui::WindowGroupConfiguration ui_config = config;
  host_->AsWindowTreeHost()->CreateGroup(ui_config);
}

void WebAppWindow::AttachToGroup(const std::string& group,
                                 const std::string& layer) {
  if (!host_)
    return;

  host_->AsWindowTreeHost()->AttachToGroup(group, layer);
}

void WebAppWindow::FocusGroupOwner() {
  if (!host_)
    return;

  host_->AsWindowTreeHost()->FocusGroupOwner();
}

void WebAppWindow::FocusGroupLayer() {
  if (!host_)
    return;

  host_->AsWindowTreeHost()->FocusGroupLayer();
}

void WebAppWindow::DetachGroup() {
  if (!host_)
    return;

  host_->AsWindowTreeHost()->DetachGroup();
}

}  // namespace app_runtime
