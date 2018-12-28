// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef UI_PLATFORM_WINDOW_WAYLAND_EXTERNAL_WAYLAND_PLATFORM_WINDOW_DELEGATE_H_
#define UI_PLATFORM_WINDOW_WAYLAND_EXTERNAL_WAYLAND_PLATFORM_WINDOW_DELEGATE_H_

#include <vector>

#include "ui/views/widget/desktop_aura/neva/ui_constants.h"

namespace ui {
// ozone-wayland additions for platform window delegate.
class WaylandPlatformWindowDelegate {
 public:
  virtual void OnDragEnter(unsigned windowhandle,
                           float x,
                           float y,
                           const std::vector<std::string>& mime_types,
                           uint32_t serial) { }

  virtual void OnDragDataReceived(int fd) { }

  virtual void OnDragLeave() { }

  virtual void OnDragMotion(float x,
                            float y,
                            uint32_t time) { }

  virtual void OnDragDrop() { }

  ///@name USE_NEVA_APPRUNTIME
  ///@{
  virtual void OnInputPanelVisibilityChanged(bool visible) { }
  virtual void OnInputPanelRectChanged(int32_t x,
                                       int32_t y,
                                       uint32_t width,
                                       uint32_t height) {}
  virtual void OnWindowHostExposed() { }
  virtual void OnWindowHostClose() { }
  virtual void OnKeyboardEnter() { }
  virtual void OnKeyboardLeave() { }
  virtual void OnWindowHostStateChanged(ui::WidgetState new_state) { }
  virtual void OnWindowHostStateAboutToChange(ui::WidgetState state) { }
  virtual void OnCursorVisibilityChange(bool visible) {}
  ///@}
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_WAYLAND_EXTERNAL_WAYLAND_PLATFORM_WINDOW_DELEGATE_H_
