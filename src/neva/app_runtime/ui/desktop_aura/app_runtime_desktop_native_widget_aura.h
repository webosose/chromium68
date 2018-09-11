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

#ifndef NEVA_APP_RUNTIME_UI_DESKTOP_AURA_APP_RUNTIME_DESKTOP_NATIVE_WIDGET_AURA_H_
#define NEVA_APP_RUNTIME_UI_DESKTOP_AURA_APP_RUNTIME_DESKTOP_NATIVE_WIDGET_AURA_H_

#include "ui/views/widget/desktop_aura/desktop_native_widget_aura.h"

namespace views {
class NativeEventDelegate;
}

namespace app_runtime {
class WebAppWindow;

class AppRuntimeDesktopNativeWidgetAura : public views::DesktopNativeWidgetAura {
 public:
  AppRuntimeDesktopNativeWidgetAura(WebAppWindow* webapp_window);

  void OnWebAppWindowRemoved();
  void SetNativeEventDelegate(views::NativeEventDelegate*);
  // Overridden from views::DesktopNativeWidgetAura:
  views::NativeEventDelegate* GetNativeEventDelegate() const override;

 protected:
  ~AppRuntimeDesktopNativeWidgetAura() override;

 private:
  WebAppWindow* webapp_window_;
  views::NativeEventDelegate* native_event_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeDesktopNativeWidgetAura);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_UI_DESKTOP_AURA_APP_RUNTIME_DESKTOP_NATIVE_WIDGET_AURA_H_