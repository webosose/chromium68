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

#include "neva/app_runtime/public/app_runtime_event.h"

#include "ui/events/event.h"

namespace app_runtime {

///////////////////////////////////////////////////////////////////////////////
// MouseEvent, public:

int MouseEvent::GetButton() {
  int buttons = ButtonNone;

  if (GetFlags() & ui::EF_LEFT_MOUSE_BUTTON)
    buttons |= ButtonLeft;
  if (GetFlags() & ui::EF_MIDDLE_MOUSE_BUTTON)
    buttons |= ButtonMiddle;
  if (GetFlags() & ui::EF_RIGHT_MOUSE_BUTTON)
    buttons |= ButtonRight;

  return buttons;
}

int MouseEvent::GetFlags() {
  return 0;
}

///////////////////////////////////////////////////////////////////////////////
// AppRuntimeMouseEvent, public:

int AppRuntimeMouseEvent::GetFlags() {
  return AppRuntimeEvent::GetFlags();
}

}  // namespace app_runtime
