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

#ifndef NEVA_APP_RUNTIME_WEBAPP_WINDOW_H_
#define NEVA_APP_RUNTIME_WEBAPP_WINDOW_H_

#include "base/timer/timer.h"
#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/public/webapp_window_base.h"
#include "ui/views/widget/desktop_aura/neva/native_event_delegate.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"
#include "ui/views/widget/widget.h"
#include "ui/views/widget/widget_delegate.h"

namespace content {
class WebContents;
}

namespace views {
class DesktopWindowTreeHost;
class WebView;
class Widget;
}

namespace app_runtime {

class AppRuntimeDesktopNativeWidgetAura;
class AppRuntimeEvent;
class WebAppWindowDelegate;
class WindowGroupConfiguration;

class WebAppWindow : public views::NativeEventDelegate,
                     public views::WidgetDelegateView {
 public:
  WebAppWindow(const WebAppWindowBase::CreateParams& params, WebAppWindowDelegate* delegate);
  ~WebAppWindow() override;

  bool Event(AppRuntimeEvent* app_runtime_event);

  void Activate();
  void SetupWebContents(content::WebContents* web_contents);
  content::WebContents* GetWebContents();
  void SetWindowHostState(ui::WidgetState state);
  ui::WidgetState GetWindowHostState() const;
  ui::WidgetState GetWindowHostStateAboutToChange() const;

  void Resize(int width, int height);
  void SetInputRegion(const std::vector<gfx::Rect>& region);
  void SetGroupKeyMask(ui::KeyMask key_mask);
  void SetKeyMask(ui::KeyMask key_mask, bool set);
  void SetOpacity(float opacity);
  void SetRootLayerOpacity(float opacity);
  void SetScaleFactor(float scale);
  void SetWindowTitle(const base::string16& title);
  void SetBounds(int x, int y, int width, int height);
  void SetCustomCursor(CustomCursorType type,
                       const std::string& path,
                       int hotspot_x,
                       int hotspot_y);
  void SetWindowProperty(const std::string& name, const std::string& value);
  void Show();
  void Hide();
  void Minimize();
  void Restore();
  void Close();
  void CloseWindow();
  void OnDesktopNativeWidgetAuraRemoved();
  bool IsInputPanelVisible() const;
  void SetUseVirtualKeyboard(bool enable);
  void CompositorResumeDrawing();
  void XInputActivate(const std::string& type);
  void XInputDeactivate();
  void XInputInvokeAction(uint32_t keysym,
                          XInputKeySymbolType symType,
                          XInputEventType eventType);
  int GetWidth() const;
  int GetHeight() const;

  // Overridden from NativeEventDelegate
  void CompositorBuffersSwapped() override;
  void CursorVisibilityChange(bool visible) override;
  void InputPanelVisibilityChanged(bool visible) override;
  void InputPanelRectChanged(int32_t x,
                             int32_t y,
                             uint32_t width,
                             uint32_t height) override;
  void KeyboardEnter() override;
  void KeyboardLeave() override;
  void WindowHostClose() override;
  void WindowHostExposed() override;
  void WindowHostStateChanged(ui::WidgetState new_state) override;
  void WindowHostStateAboutToChange(ui::WidgetState state) override;

  // Overridden from WidgetDelegateView
  views::Widget* GetWidget() override;
  const views::Widget* GetWidget() const override;
  View* GetContentsView() override;
  bool CanResize() const override;
  bool CanMaximize() const override;
  bool ShouldShowWindowTitle() const override;
  bool ShouldShowCloseButton() const override;
  base::string16 GetWindowTitle() const override;
  void WindowClosing() override;
  void CreateGroup(const WindowGroupConfiguration& config);
  void AttachToGroup(const std::string& group, const std::string& layer);
  void FocusGroupOwner();
  void FocusGroupLayer();
  void DetachGroup();
  void DeleteDelegate() override;

  // Overridden from ui::EventHandler
  void OnMouseEvent(ui::MouseEvent* event) override;
  void OnKeyEvent(ui::KeyEvent* event) override;

  // Converting values from |ui::WidgetState| to |app_runtime::WidgetState|
  static WidgetState ToExposedWidgetStateType(ui::WidgetState state);

  // Converting values from |app_runtime::WidgetState| to |ui::WidgetState|
  static ui::WidgetState FromExposedWidgetStateType(WidgetState state);

  // Converting values from |app_runtime::KeyMask| to |ui::KeyMask|
  static ui::KeyMask FromExposedKeyMaskType(KeyMask key_mask);

 protected:
  void RecreateIfNeeded();
  void SetDeferredDeleting(bool deferred_deleting) {
    deferred_deleting_ = deferred_deleting;
  }
  int input_panel_height() {
    return input_panel_rect_.height() / scale_factor_;
  }

 private:
  static views::Widget* CreateWebAppWindow(const WebAppWindowBase::CreateParams& create_params);
  void InitWindow();
  void ComputeScaleFactor();
  bool IsTextInputOverlapped();
  void UpdateViewportY();
  void UpdateViewportYCallback();

  views::Widget* widget_ = nullptr;
  views::WebView* webview_ = nullptr;
  views::DesktopWindowTreeHost* host_ = nullptr;
  WebAppWindowDelegate* delegate_;
  AppRuntimeDesktopNativeWidgetAura* desktop_native_widget_aura_ = nullptr;
  content::WebContents* web_contents_ = nullptr;
  std::map<std::string, std::string> window_property_list_;
  bool deferred_deleting_ = false;
  bool widget_closed_ = false;
  base::OneShotTimer viewport_timer_;
  gfx::Rect input_panel_rect_;
  int viewport_shift_height_ = 0;
  bool input_panel_visible_ = false;
  WebAppWindowBase::CreateParams params_;
  gfx::Rect rect_;
  float scale_factor_;

  base::string16 title_;
  ui::WidgetState window_host_state_;
  ui::WidgetState window_host_state_about_to_change_;
  DISALLOW_COPY_AND_ASSIGN(WebAppWindow);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_WEBAPP_WINDOW_H_
