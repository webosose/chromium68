// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "neva/app_runtime/public/webapp_window_base.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/browser/web_contents.h"
#include "neva/app_runtime/webapp_window.h"
#include "ui/gfx/location_hint.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "ui/views/widget/widget.h"

namespace app_runtime {

namespace {

const int kDefaultWidth = 640;
const int kDefaultHeight = 480;

gfx::LocationHint ToGfxLocationHint(
    WebAppWindowBase::LocationHint location_hint) {
  switch (location_hint) {
    case WebAppWindowBase::LocationHint::kUnknown:
      return gfx::LocationHint::kUnknown;
    case WebAppWindowBase::LocationHint::kNorth:
      return gfx::LocationHint::kNorth;
    case WebAppWindowBase::LocationHint::kWest:
      return gfx::LocationHint::kWest;
    case WebAppWindowBase::LocationHint::kSouth:
      return gfx::LocationHint::kSouth;
    case WebAppWindowBase::LocationHint::kEast:
      return gfx::LocationHint::kEast;
    case WebAppWindowBase::LocationHint::kCenter:
      return gfx::LocationHint::kCenter;
    case WebAppWindowBase::LocationHint::kNorthWest:
      return gfx::LocationHint::kNorthWest;
    case WebAppWindowBase::LocationHint::kNorthEast:
      return gfx::LocationHint::kNorthEast;
    case WebAppWindowBase::LocationHint::kSouthWest:
      return gfx::LocationHint::kSouthWest;
    case WebAppWindowBase::LocationHint::kSouthEast:
      return gfx::LocationHint::kSouthEast;
    default:
      NOTREACHED() << __func__ << "(): unknown location hint value: "
                   << static_cast<uint32_t>(location_hint);
      return gfx::LocationHint::kUnknown;
  }
}

views::Widget::InitParams::Type ToViewsWidgetType(
    WebAppWindowBase::WidgetType type) {
  switch (type) {
    case WebAppWindowBase::WidgetType::kWindow:
      return views::Widget::InitParams::TYPE_WINDOW;
    case WebAppWindowBase::WidgetType::kPanel:
      return views::Widget::InitParams::TYPE_PANEL;
    case WebAppWindowBase::WidgetType::kWindowFrameless:
      return views::Widget::InitParams::TYPE_WINDOW_FRAMELESS;
    case WebAppWindowBase::WidgetType::kControl:
      return views::Widget::InitParams::TYPE_CONTROL;
    case WebAppWindowBase::WidgetType::kPopup:
      return views::Widget::InitParams::TYPE_POPUP;
    case WebAppWindowBase::WidgetType::kMenu:
      return views::Widget::InitParams::TYPE_MENU;
    case WebAppWindowBase::WidgetType::kTooltip:
      return views::Widget::InitParams::TYPE_TOOLTIP;
    case WebAppWindowBase::WidgetType::kBubble:
      return views::Widget::InitParams::TYPE_BUBBLE;
    case WebAppWindowBase::WidgetType::kDrag:
      return views::Widget::InitParams::TYPE_DRAG;
  }

  NOTREACHED();
}

ui::WindowShowState ToUiWindowShowState(
    WebAppWindowBase::WindowShowState state) {
  switch (state) {
    case WebAppWindowBase::WindowShowState::kDefault:
      return ui::SHOW_STATE_DEFAULT;
    case WebAppWindowBase::WindowShowState::kNormal:
      return ui::SHOW_STATE_NORMAL;
    case WebAppWindowBase::WindowShowState::kMinimized:
      return ui::SHOW_STATE_MINIMIZED;
    case WebAppWindowBase::WindowShowState::kMaximized:
      return ui::SHOW_STATE_MAXIMIZED;
    case WebAppWindowBase::WindowShowState::kInactive:
      return ui::SHOW_STATE_INACTIVE;
    case WebAppWindowBase::WindowShowState::kFullscreen:
      return ui::SHOW_STATE_FULLSCREEN;
    case WebAppWindowBase::WindowShowState::kEnd:
      return ui::SHOW_STATE_END;
  }

  NOTREACHED();
}

WebAppWindow::CreateParams ToWebAppWindowParams(
    const WebAppWindowBase::CreateParams& params) {
  WebAppWindow::CreateParams result;
  result.bounds =
      gfx::Rect(params.pos_x, params.pos_y, params.width, params.height);
  result.type = ToViewsWidgetType(params.type);
  result.show_state = ToUiWindowShowState(params.show_state);
  result.web_contents = static_cast<content::WebContents*>(params.web_contents);
  result.location_hint = ToGfxLocationHint(params.location_hint);
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// WebAppWindowBase, public:

WebAppWindowBase::WebAppWindowBase() {
  CreateParams params;
  params.width = kDefaultWidth;
  params.height = kDefaultHeight;
  webapp_window_ = new WebAppWindow(ToWebAppWindowParams(params), this);
}

WebAppWindowBase::WebAppWindowBase(const CreateParams& params)
    : webapp_window_(new WebAppWindow(ToWebAppWindowParams(params), this)) {}

WebAppWindowBase::~WebAppWindowBase() {
  // As far as WebAppWindow is the instance of WidgetDelegateView,
  // it's deleted on WidgetDelegateView::DeleteDelegate().
  // We rely that WebAppWindow is deleted by views::Widget.
}

void WebAppWindowBase::Activate() {
  DCHECK(webapp_window_);
  webapp_window_->Activate();
}

void WebAppWindowBase::Show() {
  DCHECK(webapp_window_);
  webapp_window_->Show();
}

void WebAppWindowBase::Hide() {
  DCHECK(webapp_window_);
  webapp_window_->Hide();
}

void WebAppWindowBase::Restore() {
  DCHECK(webapp_window_);
  webapp_window_->Restore();
}

void WebAppWindowBase::Minimize() {
  DCHECK(webapp_window_);
  webapp_window_->Minimize();
}

void WebAppWindowBase::SetCustomCursor(CustomCursorType type,
                                       const std::string& path,
                                       int hotspot_x,
                                       int hotspot_y) {
  webapp_window_->SetCustomCursor(type, path, hotspot_x, hotspot_y);
}

void WebAppWindowBase::SetHiddenState(bool hidden) {
  NOTIMPLEMENTED();
  // if (!webapp_window_->GetObserver())
  //  return;
  // webapp_window_->GetObserver()->SetHiddenState(hidden);
}

void WebAppWindowBase::FirstFrameVisuallyCommitted() {
  NOTIMPLEMENTED();
  // if (!webapp_window_->GetObserver())
  //  return;
  // webapp_window_->GetObserver()->StartCompositor();
}

// TODO: if it is a public API we should to get rid of
//       WebContents pointer even if it's pointer to void.
void WebAppWindowBase::AttachWebContents(WebContents* web_contents) {
  DCHECK(webapp_window_);
  webapp_window_->SetupWebContents(
      static_cast<content::WebContents*>(web_contents));
}

void WebAppWindowBase::DetachWebContents() {
  webapp_window_->SetupWebContents(nullptr);
}

void WebAppWindowBase::RecreatedWebContents() {
  DCHECK(webapp_window_);
  webapp_window_->CompositorResumeDrawing();
}

WidgetState WebAppWindowBase::GetWindowHostState() const {
  DCHECK(webapp_window_);
  return WebAppWindow::ToExposedWidgetStateType(
      webapp_window_->GetWindowHostState());
}

WidgetState WebAppWindowBase::GetWindowHostStateAboutToChange() const {
  DCHECK(webapp_window_);
  return WebAppWindow::ToExposedWidgetStateType(
      webapp_window_->GetWindowHostStateAboutToChange());
}

void WebAppWindowBase::SetWindowHostState(WidgetState state) {
  DCHECK(webapp_window_);
  webapp_window_->SetWindowHostState(
      WebAppWindow::FromExposedWidgetStateType(state));
}

void WebAppWindowBase::SetInputRegion(const std::vector<gfx::Rect>& region) {
  DCHECK(webapp_window_);
  webapp_window_->SetInputRegion(region);
}

void WebAppWindowBase::SetGroupKeyMask(KeyMask key_mask) {
  DCHECK(webapp_window_);
  webapp_window_->SetGroupKeyMask(
      WebAppWindow::FromExposedKeyMaskType(key_mask));
}

void WebAppWindowBase::SetKeyMask(KeyMask key_mask, bool set) {
  DCHECK(webapp_window_);
  webapp_window_->SetKeyMask(
      WebAppWindow::FromExposedKeyMaskType(key_mask), set);
}

void WebAppWindowBase::SetWindowProperty(const std::string& name,
                                         const std::string& value) {
  webapp_window_->SetWindowProperty(name, value);
}

void WebAppWindowBase::SetLocationHint(LocationHint value) {
  webapp_window_->SetLocationHint(ToGfxLocationHint(value));
}

void WebAppWindowBase::SetOpacity(float opacity) {
  DCHECK(webapp_window_);
  webapp_window_->SetOpacity(opacity);
}

void WebAppWindowBase::SetRootLayerOpacity(float opacity) {
  DCHECK(webapp_window_);
  webapp_window_->SetRootLayerOpacity(opacity);
}

void WebAppWindowBase::Resize(int width, int height) {
  DCHECK(webapp_window_);
  webapp_window_->Resize(width, height);
}

void WebAppWindowBase::SetBounds(int x, int y, int width, int height) {
  DCHECK(webapp_window_);
  webapp_window_->SetBounds(x, y, width, height);
}

void WebAppWindowBase::SetScaleFactor(float scale) {
  DCHECK(webapp_window_);
  webapp_window_->SetScaleFactor(scale);
}

bool WebAppWindowBase::IsKeyboardVisible() const {
  return webapp_window_->IsInputPanelVisible();
}

void WebAppWindowBase::SetUseVirtualKeyboard(bool enable) {
  webapp_window_->SetUseVirtualKeyboard(enable);
}

int WebAppWindowBase::GetWidth() const {
  DCHECK(webapp_window_);
  return webapp_window_->GetWidth();
}

int WebAppWindowBase::GetHeight() const {
  DCHECK(webapp_window_);
  return webapp_window_->GetHeight();
}

void WebAppWindowBase::Close() {
  DCHECK(webapp_window_);
  webapp_window_->Close();
}

void WebAppWindowBase::OnWindowClosing() {
  webapp_window_ = nullptr;
}

void WebAppWindowBase::SetWindowTitle(const std::string& title) {
  DCHECK(webapp_window_);
  webapp_window_->SetWindowTitle(base::UTF8ToUTF16(title));
}

std::string WebAppWindowBase::GetWindowTitle() const {
  DCHECK(webapp_window_);
  return base::UTF16ToUTF8(webapp_window_->GetWindowTitle());
}

void WebAppWindowBase::XInputActivate(const std::string& type) {
  webapp_window_->XInputActivate(type);
}

void WebAppWindowBase::XInputDeactivate() {
  webapp_window_->XInputDeactivate();
}

void WebAppWindowBase::XInputInvokeAction(uint32_t keysym,
                                          XInputKeySymbolType symbol_type,
                                          XInputEventType event_type) {
  webapp_window_->XInputInvokeAction(keysym, symbol_type, event_type);
}

}  // namespace app_runtime
