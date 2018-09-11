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

#ifndef NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_CONSTANTS_H_
#define NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_CONSTANTS_H_

#include <cstdint>

#define APP_RUNTIME_PREEDIT_HLCOLOR 0xC6B0BA

namespace app_runtime {

// Platform dependent enum, revise
//enum InputPanelState {
//  INPUT_PANEL_HIDDEN = 0,
//  INPUT_PANEL_SHOWN = 1,
//  INPUT_PANEL_SIZECHANGED = 2
//};

enum NativeWindowState {
  NATIVE_WINDOW_DEFAULT = 0,
  NATIVE_WINDOW_MINIMIZED = 1,
  NATIVE_WINDOW_MAXIMIZED = 2,
  NATIVE_WINDOW_FULLSCREEN = 3
};

// Shall be kept in sync with the underlying (transfer) type(s)
// (e.g., |ui::KeyMask| enum class and its enumerated entries)
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
  kDefault = 0XFFFFFFF8
};

enum class CustomCursorType {
  kNotUse = 0,
  kBlank  = 1,
  kPath   = 2,
};

// SSL Certificate Error handling policy
enum SSLCertErrorPolicy {
  // cancel the requests and show error page
  SSL_CERT_ERROR_POLICY_DEFAULT = 0,
  // ignore error and continue requests
  SSL_CERT_ERROR_POLICY_IGNORE = 1,
  // stop all requests and do nothings
  SSL_CERT_ERROR_POLICY_DENY = 2,
};

enum XInputKeySymbolType { XINPUT_QT_KEY_SYMBOL = 1, XINPUT_NATIVE_KEY_SYMBOL };

enum XInputEventType {
  XINPUT_PRESS_AND_RELEASE = 0,
  XINPUT_PRESS,
  XINPUT_RELEASE
};

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

enum WebPageVisibilityState {
  WebPageVisibilityStateVisible,
  WebPageVisibilityStateHidden,
  WebPageVisibilityStateLaunching,
  WebPageVisibilityStatePrerender,
  WebPageVisibilityStateLast = WebPageVisibilityStatePrerender
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_CONSTANTS_H_
