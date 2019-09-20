// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_PLATFORM_WINDOW_PLATFORM_WINDOW_INIT_PROPERTIES_H_
#define UI_PLATFORM_WINDOW_PLATFORM_WINDOW_INIT_PROPERTIES_H_

#include <string>

#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"

namespace ui {

enum class PlatformWindowType {
  kWindow,
  kPopup,
  kMenu,
  kTooltip,
};

// Initial properties which are passed to PlatformWindow to be initialized
// with a desired set of properties.
struct PlatformWindowInitProperties {
  // Tells desired PlatformWindow type. It can be popup, menu or anything else.
  PlatformWindowType type = PlatformWindowType::kWindow;
  // Sets the desired initial bounds. Can be empty.
  gfx::Rect bounds;
  // Tells PlatformWindow which native widget its parent holds. It is usually
  // used to find a parent from internal list of PlatformWindows.
  gfx::AcceleratedWidget parent_widget = gfx::kNullAcceleratedWidget;

  // Surface id, which is used when ivi shell is used.
  int surface_id = 0;
};

}  // namespace ui

#endif  // UI_PLATFORM_WINDOW_PLATFORM_WINDOW_INIT_PROPERTIES_H_
