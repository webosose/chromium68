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

#ifndef UI_EVENTS_OZONE_EVDEV_KEYBOARD_EVDEV_NEVA_H_
#define UI_EVENTS_OZONE_EVDEV_KEYBOARD_EVDEV_NEVA_H_

#include <memory>

#include "ui/events/ozone/evdev/keyboard_evdev.h"

namespace ui {

class KeyboardEvdevNeva : public KeyboardEvdev {
 public:
   KeyboardEvdevNeva(EventModifiers* modifiers,
                     KeyboardLayoutEngine* keyboard_layout_engine,
                     const EventDispatchCallback& callback)
    : KeyboardEvdev(modifiers, keyboard_layout_engine, callback) {}

   virtual ~KeyboardEvdevNeva() {}

   static std::unique_ptr<KeyboardEvdevNeva> Create(
                     EventModifiers* modifiers,
                     KeyboardLayoutEngine* keyboard_layout_engine,
                     const EventDispatchCallback& callback);

 private:
  DISALLOW_COPY_AND_ASSIGN(KeyboardEvdevNeva);
};

} // namespace ui
#endif  // UI_EVENTS_OZONE_EVDEV_KEYBOARD_EVDEV_NEVA_H_
