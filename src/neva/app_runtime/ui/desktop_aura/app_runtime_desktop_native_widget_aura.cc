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

#include "neva/app_runtime/ui/desktop_aura/app_runtime_desktop_native_widget_aura.h"
#include "neva/app_runtime/webapp_window.h"
#include "ui/aura/client/drag_drop_client.h"
#include "ui/aura/window_tree_host.h"
#include "ui/wm/public/scoped_tooltip_disabler.h"

namespace app_runtime {

AppRuntimeDesktopNativeWidgetAura::AppRuntimeDesktopNativeWidgetAura(
    WebAppWindow* webapp_window)
    : views::DesktopNativeWidgetAura(webapp_window->GetWidget()),
      webapp_window_(webapp_window),
      native_event_delegate_(nullptr) {
}

AppRuntimeDesktopNativeWidgetAura::~AppRuntimeDesktopNativeWidgetAura() {
  if (webapp_window_)
    webapp_window_->OnDesktopNativeWidgetAuraRemoved();
}

void AppRuntimeDesktopNativeWidgetAura::OnWebAppWindowRemoved() {
  webapp_window_ = nullptr;
}

void AppRuntimeDesktopNativeWidgetAura::SetNativeEventDelegate(
    views::NativeEventDelegate* native_event_delegate) {
  native_event_delegate_ = native_event_delegate;
}

views::NativeEventDelegate* AppRuntimeDesktopNativeWidgetAura::GetNativeEventDelegate() const {
  return native_event_delegate_;
}

void AppRuntimeDesktopNativeWidgetAura::InitNativeWidget(
    const views::Widget::InitParams& params) {
  DesktopNativeWidgetAura::InitNativeWidget(params);
  aura::client::SetDragDropClient(host()->window(), nullptr);
  tooltip_disabler_.reset(
      new wm::ScopedTooltipDisabler(GetNativeView()->GetRootWindow()));
}

}  // namespace app_runtime
