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

#include "webos/webapp_window_base.h"

#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/public/window_group_configuration.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_window_tree_host.h"
#include "webos/webapp_window.h"
#include "webos/window_group_configuration.h"
#include "webos/common/webos_types_conversion_utils.h"

namespace webos {

namespace {

app_runtime::CustomCursorType ToAppruntimeCursorType(CustomCursorType type) {
  switch (type) {
    case CUSTOM_CURSOR_NOT_USE :
      return app_runtime::CustomCursorType::kNotUse;
    case CUSTOM_CURSOR_BLANK :
      return app_runtime::CustomCursorType::kBlank;
    case CUSTOM_CURSOR_PATH :
      return app_runtime::CustomCursorType::kPath;
  }

  NOTREACHED();
}

}  // namespace

////////////////////////////////////////////////////////////////////////////////
// WebAppWindowBase, public:

WebAppWindowBase::WebAppWindowBase() {}

WebAppWindowBase::~WebAppWindowBase() {
  if (webapp_window_) {
    webapp_window_->SetDelegate(nullptr);
    webapp_window_->Close();
  }
}

void WebAppWindowBase::InitWindow(int width, int height) {
  if (webapp_window_)
    webapp_window_->Close();

  app_runtime::WebAppWindowBase::CreateParams params;
  params.width = width;
  params.height = height;
  params.show_state = app_runtime::WebAppWindowBase::CreateParams::WindowShowState::kFullscreen;
  params.type = app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindowFrameless;
  webapp_window_ = new WebAppWindow(params);
  webapp_window_->SetDelegate(this);
}

void WebAppWindowBase::Show() {
  if (webapp_window_)
    webapp_window_->ActivateAndShow();
}

void WebAppWindowBase::Hide() {
  if (webapp_window_)
    webapp_window_->CloseWindow();
}

// TODO(sergey.kipet@lge.com): deprecated (to be excluded as soon as removed
// from every component it is called from, e.g. WAM)
unsigned WebAppWindowBase::GetWindowHandle() {
  NOTIMPLEMENTED() << "WebOS specific api of DesktopTreeWindowHostOzone";
  return 0;
}

void WebAppWindowBase::SetCustomCursor(CustomCursorType type,
                                       const std::string& path,
                                       int hotspot_x,
                                       int hotspot_y) {
  webapp_window_->SetCustomCursor(ToAppruntimeCursorType(type), path,
                                  hotspot_x, hotspot_y);
}

int WebAppWindowBase::DisplayWidth() const {
  if (display::Screen::GetScreen()->GetNumDisplays() > 0)
    return display::Screen::GetScreen()->GetPrimaryDisplay().bounds().width();
  return 0;
}

int WebAppWindowBase::DisplayHeight() const {
  if (display::Screen::GetScreen()->GetNumDisplays() > 0)
    return display::Screen::GetScreen()->GetPrimaryDisplay().bounds().height();
  return 0;
}

void WebAppWindowBase::AttachWebContents(void* web_contents) {
  if (webapp_window_) {
    webapp_window_->SetupWebContents(
        static_cast<content::WebContents*>(web_contents));
  }
}

void WebAppWindowBase::DetachWebContents() {
  if (webapp_window_)
    webapp_window_->SetupWebContents(nullptr);
}

void WebAppWindowBase::RecreatedWebContents() {
  if (webapp_window_)
    webapp_window_->CompositorResumeDrawing();
}

NativeWindowState WebAppWindowBase::GetWindowHostState() const {
  if (webapp_window_)
    return ToNativeWindowState(webapp_window_->GetWindowHostState());

  return NATIVE_WINDOW_DEFAULT;
}

NativeWindowState WebAppWindowBase::GetWindowHostStateAboutToChange() const {
  if (webapp_window_)
    return ToNativeWindowState(
        webapp_window_->GetWindowHostStateAboutToChange());

  return NATIVE_WINDOW_DEFAULT;
}

void WebAppWindowBase::SetWindowHostState(NativeWindowState state) {
  if (webapp_window_)
    webapp_window_->SetWindowHostState(ToWidgetState(state));
}

void WebAppWindowBase::SetInputRegion(const std::vector<gfx::Rect>& region) {
  if (webapp_window_)
    webapp_window_->SetInputRegion(region);
}

void WebAppWindowBase::SetKeyMask(WebOSKeyMask key_mask) {
  if (webapp_window_)
    webapp_window_->SetGroupKeyMask(ToKeyMask(key_mask));
}

void WebAppWindowBase::SetKeyMask(WebOSKeyMask key_mask, bool set) {
  if (webapp_window_)
    webapp_window_->SetKeyMask(ToKeyMask(key_mask), set);
}

void WebAppWindowBase::SetWindowProperty(const std::string& name,
                                         const std::string& value) {
  if (webapp_window_)
    webapp_window_->SetWindowProperty(name, value);
}

void WebAppWindowBase::SetOpacity(float opacity) {
  // SetRootLayerOpacity instead of SetOpacity should be called for WebOS
  if (webapp_window_)
    webapp_window_->SetRootLayerOpacity(opacity);
}

void WebAppWindowBase::Resize(int width, int height) {
  if (webapp_window_)
    webapp_window_->Resize(width, height);
}

void WebAppWindowBase::SetScaleFactor(float scale) {
  if (webapp_window_)
    webapp_window_->SetScaleFactor(scale);
}

bool WebAppWindowBase::IsKeyboardVisible() {
  return webapp_window_->IsInputPanelVisible();
}

void WebAppWindowBase::SetUseVirtualKeyboard(bool enable) {
  if (webapp_window_)
    webapp_window_->SetUseVirtualKeyboard(enable);
}

void WebAppWindowBase::CreateWindowGroup(
    const WindowGroupConfiguration& config) {
  app_runtime::WindowGroupConfiguration ui_config;

  ui_config.name = config.GetName();
  ui_config.is_anonymous = config.GetIsAnonymous();

  for (auto& it : config.GetLayers()) {
    app_runtime::WindowGroupLayerConfiguration layer_config;
    layer_config.name = it.GetName();
    layer_config.z_order = it.GetZOrder();
    ui_config.layers.push_back(layer_config);
  }
  webapp_window_->CreateGroup(ui_config);
}

void WebAppWindowBase::AttachToWindowGroup(const std::string& group,
                                           const std::string& layer) {
  webapp_window_->AttachToGroup(group, layer);
}

void WebAppWindowBase::FocusWindowGroupOwner() {
  webapp_window_->FocusGroupOwner();
}

void WebAppWindowBase::FocusWindowGroupLayer() {
  webapp_window_->FocusGroupLayer();
}

void WebAppWindowBase::DetachWindowGroup() {
  webapp_window_->DetachGroup();
}

void WebAppWindowBase::OnWindowClosing() {
}

void WebAppWindowBase::XInputActivate(const std::string& type) {
  NOTIMPLEMENTED() << "WebOS specific api of DesktopWindowTreeHostOzone";
}

void WebAppWindowBase::XInputDeactivate() {
  NOTIMPLEMENTED() << "WebOS specific api of DesktopWindowTreeHostOzone";
}

void WebAppWindowBase::XInputInvokeAction(uint32_t keysym,
                                          SpecialKeySymbolType symbol_type,
                                          XInputEventType event_type) {
  NOTIMPLEMENTED() << "WebOS specific api of DesktopWindowTreeHostOzone";
}

void WebAppWindowBase::WebAppWindowDestroyed() {
  webapp_window_ = nullptr;
}

}  // namespace webos
