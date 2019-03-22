// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2014 Intel Corporation. All rights reserved.
// Copyright 2016 LG Electronics, Inc.
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

#ifndef UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_UI_CONSTANTS_H_
#define UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_UI_CONSTANTS_H_

#include <cstdint>

namespace ui {

enum class WidgetState {
  UNINITIALIZED = 0,
  SHOW = 1,  // Widget is visible.
  HIDE = 2,  // Widget is hidden.
  FULLSCREEN = 3,  // Widget is in fullscreen mode.
  MAXIMIZED = 4,  // Widget is maximized,
  MINIMIZED = 5,  // Widget is minimized.
  RESTORE = 6,  // Restore Widget.
  ACTIVE = 7,  // Widget is Activated.
  INACTIVE = 8,  // Widget is DeActivated.
  RESIZE = 9,  // Widget is Resized.
  DESTROYED = 10  // Widget is Destroyed.
};

enum class WidgetType {
  WINDOW = 1,  // A decorated Window.
  WINDOWFRAMELESS = 2,  // An undecorated Window.
  POPUP = 3,  // An undecorated Window, with transient positioning relative to
              // its parent and in which the input pointer is implicit grabbed
              // (i.e. Wayland install the grab) by the Window.
  TOOLTIP = 4
};

enum class KeyMask : std::uint32_t {
  kHome = 1,
  kBack = 1 << 1,
  kExit = 1 << 2,
  kNavigationLeft = 1 << 3,
  kNavigationRight = 1 << 4,
  kNavigationUp = 1 << 5,
  kNavigationDown = 1 << 6,
  kNavigationOk = 1 << 7,
  kNumericKeys = 1 << 8,
  kRemoteColorRed = 1 << 9,
  kRemoteColorGreen = 1 << 10,
  kRemoteColorYellow = 1 << 11,
  kRemoteColorBlue = 1 << 12,
  kRemoteProgrammeGroup = 1 << 13,
  kRemotePlaybackGroup = 1 << 14,
  kRemoteTeletextGroup = 1 << 15,
  kLocalLeft = 1 << 16,
  kLocalRight = 1 << 17,
  kLocalUp = 1 << 18,
  kLocalDown = 1 << 19,
  kLocalOk = 1 << 20,
  kRemoteMagnifierGroup = 1 << 21,
  kMinimalPlaybackGroup = 1 << 22,
  kGuide = 1 << 23,
  kDefault = 0xFFFFFFF8
};

}  // namespace ui

#endif  // UI_VIEWS_WIDGET_DESKTOP_AURA_NEVA_UI_CONSTANTS_H_
