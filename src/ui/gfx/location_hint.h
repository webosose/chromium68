// Copyright 2019 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_LOCATION_HINT_H_
#define UI_GFX_LOCATION_HINT_H_

namespace gfx {

enum class LocationHint {
  kUnknown,
  kNorth,
  kWest,
  kSouth,
  kEast,
  kCenter,
  kNorthWest,
  kNorthEast,
  kSouthWest,
  kSouthEast
};

}  // namespace gfx

#endif  // UI_GFX_LOCATION_HINT_H_
