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

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_NATIVE_EVENT_DELEGATE_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_NATIVE_EVENT_DELEGATE_H_

#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

namespace views {

class NativeEventDelegate {
 public:
  NativeEventDelegate() {}
  virtual ~NativeEventDelegate() {}

  virtual void CompositorBuffersSwapped() = 0;
  virtual void CursorVisibilityChange(bool visible) = 0;
  virtual void InputPanelVisibilityChanged(bool visible) = 0;
  virtual void InputPanelRectChanged(int32_t x,
                                     int32_t y,
                                     uint32_t width,
                                     uint32_t height) = 0;
  virtual void KeyboardEnter() = 0;
  virtual void KeyboardLeave() = 0;
  virtual void WindowHostClose() = 0;
  virtual void WindowHostExposed() = 0;
  virtual void WindowHostStateChanged(ui::WidgetState new_state) = 0;
  virtual void WindowHostStateAboutToChange(ui::WidgetState state) = 0;
};

}  // namespace views

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_NATIVE_EVENT_DELEGATE_H_
