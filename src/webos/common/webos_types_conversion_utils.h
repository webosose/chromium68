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

#ifndef WEBOS_COMMON_WEBOS_TYPES_CONVERSION_UTILS_H_
#define WEBOS_COMMON_WEBOS_TYPES_CONVERSION_UTILS_H_

#include <climits>
#include <cstdint>

#include "base/logging.h"
#include "ui/views/widget/desktop_aura/neva/ui_constants.h"
#include "webos/common/webos_constants.h"

namespace webos {

namespace {

const std::size_t kUInt32BitLength = sizeof(std::uint32_t) * CHAR_BIT;

inline NativeWindowState ToNativeWindowState(ui::WidgetState state) {
  switch (state) {
    case ui::WidgetState::MINIMIZED:
      return NativeWindowState::NATIVE_WINDOW_MINIMIZED;
    case ui::WidgetState::MAXIMIZED:
      return NativeWindowState::NATIVE_WINDOW_MAXIMIZED;
    case ui::WidgetState::FULLSCREEN:
      return NativeWindowState::NATIVE_WINDOW_FULLSCREEN;
    default:
      return NativeWindowState::NATIVE_WINDOW_DEFAULT;
  }
}

inline ui::WidgetState ToWidgetState(NativeWindowState state) {
  switch (state) {
    case NativeWindowState::NATIVE_WINDOW_MINIMIZED:
      return ui::WidgetState::MINIMIZED;
    case NativeWindowState::NATIVE_WINDOW_MAXIMIZED:
      return ui::WidgetState::MAXIMIZED;
    case NativeWindowState::NATIVE_WINDOW_FULLSCREEN:
      return ui::WidgetState::FULLSCREEN;
    default:
      return ui::WidgetState::UNINITIALIZED;
  }
}

inline bool ExistsInUiKeyMaskType(std::uint32_t key_mask) {
  ui::KeyMask key_masks = static_cast<ui::KeyMask>(key_mask);
  switch (key_masks) {
    case ui::KeyMask::kHome:
    case ui::KeyMask::kBack:
    case ui::KeyMask::kExit:
    case ui::KeyMask::kNavigationLeft:
    case ui::KeyMask::kNavigationRight:
    case ui::KeyMask::kNavigationUp:
    case ui::KeyMask::kNavigationDown:
    case ui::KeyMask::kNavigationOk:
    case ui::KeyMask::kNumericKeys:
    case ui::KeyMask::kRemoteColorRed:
    case ui::KeyMask::kRemoteColorGreen:
    case ui::KeyMask::kRemoteColorYellow:
    case ui::KeyMask::kRemoteColorBlue:
    case ui::KeyMask::kRemoteProgrammeGroup:
    case ui::KeyMask::kRemotePlaybackGroup:
    case ui::KeyMask::kRemoteTeletextGroup:
    case ui::KeyMask::kLocalLeft:
    case ui::KeyMask::kLocalRight:
    case ui::KeyMask::kLocalUp:
    case ui::KeyMask::kLocalDown:
    case ui::KeyMask::kLocalOk:
    case ui::KeyMask::kRemoteMagnifierGroup:
    case ui::KeyMask::kMinimalPlaybackGroup:
    case ui::KeyMask::kGuide:
      return true;
    default:
      LOG(WARNING) << __func__ << "(): unknown key mask value: " << key_mask;
      return false;
  }
}

inline ui::KeyMask ToKeyMask(WebOSKeyMask key_mask) {
  if (key_mask == WebOSKeyMask::KEY_MASK_DEFAULT)
    return ui::KeyMask::kDefault;

  std::uint32_t key_masks = static_cast<std::uint32_t>(key_mask);
  std::uint32_t result = 0;

  for (std::size_t i = 0; i < kUInt32BitLength; ++i) {
    std::uint32_t mask = 1 << i;
    if ((key_masks & mask) && ExistsInUiKeyMaskType(mask))
      result |= mask;
  }

  return static_cast<ui::KeyMask>(result);
}

}  // namespace

}  // namespace webos

#endif  // WEBOS_COMMON_WEBOS_TYPES_CONVERSION_UTILS_H_
