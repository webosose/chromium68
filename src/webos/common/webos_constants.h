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

#ifndef WEBOS_COMMON_WEBOS_CONSTANTS_H_
#define WEBOS_COMMON_WEBOS_CONSTANTS_H_

#include "ozone/platform/webos_constants.h"

namespace webos {

enum SpecialKeySymbolType { QT_KEY_SYMBOL = 1, NATIVE_KEY_SYMBOL };

enum XInputEventType {
  XINPUT_PRESS_AND_RELEASE = 0,
  XINPUT_PRESS,
  XINPUT_RELEASE
};

// WebRTC forced peer connection drop reason
enum DropPeerConnectionReason {
  // Dropped because page got hidden
  DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN = 0,
  DROP_PEER_CONNECTION_REASON_UNKNOWN,
  DROP_PEER_CONNECTION_REASON_LAST = DROP_PEER_CONNECTION_REASON_UNKNOWN,
};

}  // namespace webos

#endif  // WEBOS_COMMON_WEBOS_CONSTANTS_H_
