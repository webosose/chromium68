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

#ifndef MEDIA_BASE_NEVA_VIDEO_UTIL_NEVA_H_
#define MEDIA_BASE_NEVA_VIDEO_UTIL_NEVA_H_

#include "media/base/media_export.h"
#include "ui/gfx/geometry/point_f.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/size.h"

namespace media {

// Input:
//  1) video_rect_in_view_space
//   (0, 0)
//   ---------------------------
//   |   -------------         |
//   |   |           |         |
//   |   |           |         |
//   |   -------------         |
//   |   video_rect            |
//   |                         |
//   |                         |
//   --------------------------- (view's width, height)
//
//   Note that video_rect can be different with rect of the corresponding
//   element when object-fit property is provided.
//   For example, let a video's size is (640, 480), and the video's object-fit
//   value is 'cover', and its corresponding element size is (200, 400). Then,
//
//     (0, 0)
//     ---------------------------
//     |   -------------         |
//     |   |    ---    |         |
//     |   |    | |    |         |
//     |   |    | |    |         |
//     |   |    ---element       |
//     |   -------------         |
//     |   video_rect            |
//     |                         |
//     |                         |
//     ---------------------------
//
//   We always try to fit into video_rect. Then exposed played video from
//   element area is expected result.
//
//  2) natural_video_size: visible width and height of a video frame
//
//  3) additional_scale: scale value for accounting into source_rect
//                       and visible_rect
//
//  4) view_rect_in_screen_space
//   (0, 0)
//   -------------------------------------
//   |              widget               |
//   |    ---------------------------    |
//   |    |      (browser UI)       |    |
//   |    ---------------------------    |
//   |    |(view's x, y)            |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    ---------------------------    |
//   |         (view's width, height)    |
//   |                                   |
//   ------------------------------------- (screen width, height)
//
// Output:
//  1) source_rect: source rect in natural video space.
//   (0, 0)
//   ---------------------------
//   |   -------------         |
//   |   |           |         |
//   |   |           |         |
//   |   -------------         |
//   |   source_rect           |
//   |                         |
//   |                         |
//   --------------------------- (natural video's width, height)
//
//  2) visible_rect: visible rect in screen space.
//   (0, 0)
//   -------------------------------------
//   |              widget               |
//   |    ---------------------------    |
//   |    |      (browser UI)       |    |
//   |    ---------------------------    |
//   |    |(view's x, y)            |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    |   -------------         |    |
//   |    |   |           |         |    |
//   |    |   |           |         |    |
//   |    |   -------------         |    |
//   |    |   visible_rect          |    |
//   |    |                         |    |
//   |    |                         |    |
//   |    ---------------------------    |
//   |         (view's width, height)    |
//   |                                   |
//   ------------------------------------- (screen width, height)
//
//  3) is_fullscreen: indicates fullscreen.
//
// Return value:
//  true - if computing results are different with previous results.
//  false - in opposite case
bool ComputeVideoHoleDisplayRect(
    const gfx::Rect& video_rect_in_view_space,
    const gfx::Size& natural_video_size,
    const gfx::PointF& additional_scale,
    const gfx::Rect& view_rect,
    const gfx::Rect& screen_rect,
    gfx::Rect& source_rect,
    gfx::Rect& visible_rect,
    bool& is_fullscreen);

}  // namespace media

#endif  // MEDIA_BASE_NEVA_VIDEO_UTIL_NEVA_H_