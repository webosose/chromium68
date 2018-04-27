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

#ifndef WEBOS_WEBAPP_WINDOW_BASE_H_
#define WEBOS_WEBAPP_WINDOW_BASE_H_

#include <memory>
#include <vector>

#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/public/webapp_window_delegate.h"
#include "ui/gfx/geometry/rect.h"
#include "webos/webapp_window_delegate.h"
#include "webos/common/webos_constants.h"
#include "webos/common/webos_event.h"
#include "webos/common/webos_export.h"

namespace app_runtime {
class AppRuntimeEvent;
}  // namespace app_runtime

namespace webos {
class WebAppWindow;
class WindowGroupConfiguration;

class WEBOS_EXPORT WebAppWindowBase : public WebAppWindowDelegate {
 public:
  WebAppWindowBase();
  virtual ~WebAppWindowBase();

  void Show();
  void Hide();

  void SetCustomCursor(CustomCursorType type,
                       const std::string& path,
                       int hotspot_x,
                       int hotspot_y);
  int DisplayWidth() const;
  int DisplayHeight() const;

  void AttachWebContents(void* web_contents);
  void DetachWebContents();
  void RecreatedWebContents();

  NativeWindowState GetWindowHostState() const;
  NativeWindowState GetWindowHostStateAboutToChange() const;
  void SetWindowHostState(NativeWindowState state);
  unsigned GetWindowHandle();
  void SetInputRegion(const std::vector<gfx::Rect>& region);
  void SetKeyMask(WebOSKeyMask key_mask);
  void SetKeyMask(WebOSKeyMask key_mask, bool set);
  void SetWindowProperty(const std::string& name, const std::string& value);
  void SetWindowSurfaceId(int surface_id);
  void SetOpacity(float opacity);
  void Resize(int width, int height);
  void SetScaleFactor(float scale);
  bool IsKeyboardVisible();
  void SetUseVirtualKeyboard(bool enable);
  void InitWindow(int width, int height);

  void CreateWindowGroup(const WindowGroupConfiguration& config);
  void AttachToWindowGroup(const std::string& name, const std::string& layer);
  void FocusWindowGroupOwner();
  void FocusWindowGroupLayer();
  void DetachWindowGroup();

  // Overridden from WebAppWindowObserver
  void OnWindowClosing() override;

  void XInputActivate(const std::string& type = std::string());
  void XInputDeactivate();
  void XInputInvokeAction(uint32_t keysym,
                          SpecialKeySymbolType symType = QT_KEY_SYMBOL,
                          XInputEventType eventType = XINPUT_PRESS_AND_RELEASE);

  // Overriden from webos::WebAppWindowDelegate
  void WebAppWindowDestroyed() override;

 private:
  WebAppWindow* webapp_window_ = nullptr;
};

}  // namespace webos

#endif  // WEBOS_WEBAPP_WINDOW_BASE_H_
