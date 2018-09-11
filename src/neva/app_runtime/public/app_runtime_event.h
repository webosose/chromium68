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

#ifndef NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EVENT_H_
#define NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EVENT_H_

#include "neva/app_runtime/public/app_runtime_export.h"

namespace app_runtime {

class APP_RUNTIME_EXPORT AppRuntimeEvent {
 public:
  enum Type {
    None,
    Close,
    Expose,
    WindowStateChange,
    WindowStateAboutToChange,
    KeyPress,
    KeyRelease,
    MouseMove,
    MouseButtonPress,
    MouseButtonRelease,
    Wheel,
    Enter,
    Leave,
    Swap,
    FocusIn,
    FocusOut,
    InputPanelVisible,
  };

  explicit AppRuntimeEvent(Type type, int flags = 0)
      : type_(type), flags_(flags) {}
  virtual ~AppRuntimeEvent() {}

  inline Type GetType() { return type_; }
  inline void SetType(Type type) { type_ = type; }
  inline int GetFlags() { return flags_; }
  inline void SetFlags(int flags) { flags_ = flags; }

 protected:
  Type type_;
  int flags_;
};

class APP_RUNTIME_EXPORT KeyEvent {
 public:
  explicit KeyEvent(unsigned code) : code_(code) {}
  virtual ~KeyEvent() {}

  unsigned GetCode() const { return code_; }

 protected:
  unsigned code_;
};

class APP_RUNTIME_EXPORT AppRuntimeKeyEvent : public AppRuntimeEvent,
                                              public KeyEvent {
 public:
  explicit AppRuntimeKeyEvent(AppRuntimeEvent::Type type, unsigned code)
      : AppRuntimeEvent(type), KeyEvent(code) {}
  ~AppRuntimeKeyEvent() override {}
};

class APP_RUNTIME_EXPORT MouseEvent {
 public:
  enum Button {
    ButtonNone = 0,
    ButtonLeft = 1 << 0,
    ButtonMiddle = 1 << 1,
    ButtonRight = 1 << 2,
  };

  explicit MouseEvent(float x, float y) : x_(x), y_(y) {}
  virtual ~MouseEvent() {}

  float GetX() { return x_; }
  float GetY() { return y_; }

  int GetButton();
  virtual int GetFlags();

 protected:
  float x_;
  float y_;
};

class APP_RUNTIME_EXPORT AppRuntimeMouseEvent
    : public AppRuntimeEvent,
      public MouseEvent {
 public:
  explicit AppRuntimeMouseEvent(AppRuntimeEvent::Type type,
                                float x,
                                float y,
                                int flags = 0)
      : AppRuntimeEvent(type, flags), MouseEvent(x, y) {}
  ~AppRuntimeMouseEvent() override {}

  int GetFlags() override;
};

class APP_RUNTIME_EXPORT MouseWheelEvent {
 public:
  explicit MouseWheelEvent(float x_offset, float y_offset)
      : x_offset_(x_offset), y_offset_(y_offset) {}
  virtual ~MouseWheelEvent() {}

  float GetXOffset() { return x_offset_; }
  float GetYOffset() { return y_offset_; }

 protected:
  float x_offset_;
  float y_offset_;
};

class APP_RUNTIME_EXPORT AppRuntimeMouseWheelEvent
    : public AppRuntimeMouseEvent,
      public MouseWheelEvent {
 public:
  explicit AppRuntimeMouseWheelEvent(AppRuntimeEvent::Type type,
                                     float x,
                                     float y,
                                     float x_offset,
                                     float y_offset)
      : AppRuntimeMouseEvent(type, x, y),
        MouseWheelEvent(x_offset, y_offset) {}
  ~AppRuntimeMouseWheelEvent() override {}
};

class APP_RUNTIME_EXPORT VirtualKeyboardEvent {
 public:
  explicit VirtualKeyboardEvent(bool visible, int height)
      : visible_(visible), height_(height) {}
  virtual ~VirtualKeyboardEvent() {}

  bool GetVisible() { return visible_; }
  int GetHeight() { return height_; }

 protected:
  bool visible_;
  float height_;
};

class APP_RUNTIME_EXPORT AppRuntimeVirtualKeyboardEvent
    : public AppRuntimeEvent,
      public VirtualKeyboardEvent {
 public:
  explicit AppRuntimeVirtualKeyboardEvent(Type type,
                                          bool visible,
                                          int height,
                                          int flags = 0)
      : AppRuntimeEvent(type, flags),
        VirtualKeyboardEvent(visible, height) {}
  ~AppRuntimeVirtualKeyboardEvent() override {}
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_APP_RUNTIME_EVENT_H_
