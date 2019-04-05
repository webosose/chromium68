// Copyright (c) 2018 LG Electronics, Inc.
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

#include "webos/webapp_window.h"

#include "webos/common/webos_event.h"
#include "webos/public/runtime.h"
#include "webos/webapp_window_delegate.h"

namespace webos {

WebAppWindow::WebAppWindow(const app_runtime::WebAppWindowBase::CreateParams& params)
    : app_runtime::WebAppWindow(params, nullptr),
      keyboard_enter_(false) {
  SetDeferredDeleting(true);
}

WebAppWindow::~WebAppWindow() {
  if (webapp_window_delegate_)
    webapp_window_delegate_->WebAppWindowDestroyed();
}

void WebAppWindow::SetDelegate(WebAppWindowDelegate* webapp_window_delegate) {
  webapp_window_delegate_ = webapp_window_delegate;
}

bool WebAppWindow::Event(WebOSEvent* webos_event) {
  if (webapp_window_delegate_)
    return webapp_window_delegate_->event(webos_event);

  return false;
}

void WebAppWindow::CompositorBuffersSwapped() {
  NOTIMPLEMENTED();
}

void WebAppWindow::CursorVisibilityChange(bool visible) {
  app_runtime::WebAppWindow::CursorVisibilityChange(visible);
  Runtime::GetInstance()->OnCursorVisibilityChanged(visible);
}

void WebAppWindow::InputPanelVisibilityChanged(bool visibility) {
  app_runtime::WebAppWindow::InputPanelVisibilityChanged(visibility);
  WebOSVirtualKeyboardEvent webos_event(WebOSEvent::Type::InputPanelVisible,
                                        visibility,
                                        visibility ? input_panel_height() : 0);
  Event(&webos_event);
}

void WebAppWindow::InputPanelRectChanged(int32_t x,
                                         int32_t y,
                                         uint32_t width,
                                         uint32_t height) {
  app_runtime::WebAppWindow::InputPanelRectChanged(x, y, width, height);
}

void WebAppWindow::KeyboardEnter() {
  if (keyboard_enter_)
    return;

  keyboard_enter_ = true;

  WebOSEvent webos_event(WebOSEvent::Type::FocusIn);
  Event(&webos_event);
}

void WebAppWindow::KeyboardLeave() {
  if (!keyboard_enter_)
    return;

  keyboard_enter_ = false;

  WebOSEvent webos_event(WebOSEvent::Type::FocusOut);
  Event(&webos_event);
}

void WebAppWindow::WindowHostClose() {
  WebOSEvent webos_event(WebOSEvent::Close);
  Event(&webos_event);
}

void WebAppWindow::WindowHostExposed() {
  WebOSEvent webos_event(WebOSEvent::Type::Expose);
  Event(&webos_event);
}

void WebAppWindow::WindowHostStateChanged(ui::WidgetState new_state) {
  app_runtime::WebAppWindow::WindowHostStateChanged(new_state);

  WebOSEvent webos_event(WebOSEvent::Type::WindowStateChange);
  Event(&webos_event);
}

void WebAppWindow::WindowHostStateAboutToChange(ui::WidgetState state) {
  app_runtime::WebAppWindow::WindowHostStateAboutToChange(state);

  WebOSEvent webos_event(WebOSEvent::Type::WindowStateAboutToChange);
  Event(&webos_event);
}

void WebAppWindow::OnMouseEvent(ui::MouseEvent* event) {
  app_runtime::WebAppWindow::OnMouseEvent(event);
}

void WebAppWindow::OnKeyEvent(ui::KeyEvent* event) {
  switch (event->type()) {
    case ui::EventType::ET_KEY_PRESSED:
      if (OnKeyPressed(event->key_code()))
        event->StopPropagation();
      break;
    case ui::EventType::ET_KEY_RELEASED:
      if (OnKeyReleased(event->key_code()))
        event->StopPropagation();
      break;
    default:
      break;
  }
}

bool WebAppWindow::OnKeyPressed(unsigned keycode) {
  WebOSKeyEvent event(WebOSEvent::Type::KeyPress, keycode);
  return Event(&event);
}

bool WebAppWindow::OnKeyReleased(unsigned keycode) {
  WebOSKeyEvent event(WebOSKeyEvent::Type::KeyRelease, keycode);
  return Event(&event);
}

void WebAppWindow::ActivateAndShow() {
  RecreateIfNeeded();
  Activate();
  Show();
}

}  // namespace webos
