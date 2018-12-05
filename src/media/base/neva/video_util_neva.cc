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

#include "media/base/neva/video_util_neva.h"

namespace media {

bool ComputeVideoHoleDisplayRect(const gfx::Rect& video_rect_in_view_space,
                                 const gfx::Size& natural_video_size,
                                 const gfx::PointF& additional_scale,
                                 const gfx::Rect& view_rect,
                                 const gfx::Rect& screen_rect,
                                 gfx::Rect& source_rect,
                                 gfx::Rect& visible_rect,
                                 bool& is_fullscreen) {
  // Step1: Save previous results. These values are used at last step.
  gfx::Rect prev_source_rect = source_rect;
  gfx::Rect prev_visible_rect = visible_rect;
  bool prev_is_fullscreen = is_fullscreen;

  // Step2: Compute source_rect.
  gfx::Rect view_rect_from_origin(0, 0, view_rect.width(), view_rect.height());
  visible_rect =
      gfx::IntersectRects(video_rect_in_view_space, view_rect_from_origin);

  // We check video_rect is partially visible or not. If video_rect is partially
  // visible, source_rect will be part of natural video's rect. Otherwise, it
  // will be identical with natural video's rect.
  gfx::Rect natural_video_rect(0, 0, natural_video_size.width(),
                               natural_video_size.height());
  if (visible_rect == video_rect_in_view_space || visible_rect.IsEmpty()) {
    source_rect = natural_video_rect;
  } else {
    DCHECK(video_rect_in_view_space.width() != 0 &&
           video_rect_in_view_space.height() != 0);

    int source_x = visible_rect.x() - video_rect_in_view_space.x();
    int source_y = visible_rect.y() - video_rect_in_view_space.y();

    // TODO(neva): Make sure when video scales are different between natural
    // video and display rect. Currently we try to fully fit into display rect.
    float scale_width = static_cast<float>(natural_video_rect.width()) /
                        video_rect_in_view_space.width();
    float scale_height = static_cast<float>(natural_video_rect.height()) /
                         video_rect_in_view_space.height();

    source_rect = gfx::Rect(source_x, source_y, visible_rect.width(),
                            visible_rect.height());
    source_rect =
        ScaleToEnclosingRectSafe(source_rect, scale_width, scale_height);
  }

  // Step3: Adjust visible_rect to view offset.
  visible_rect.Offset(view_rect.x(), view_rect.y());

  // Step4: Adjust visible_rect to screen space.
  visible_rect = ScaleToEnclosingRectSafe(visible_rect, additional_scale.x(),
                                          additional_scale.y());

  // Step5: Determine is_fullscreen.
  is_fullscreen =
      source_rect == natural_video_rect && visible_rect == screen_rect;

  // Step6: Check update.
  bool need_update =
      !visible_rect.IsEmpty() &&
      (prev_source_rect != source_rect || prev_visible_rect != visible_rect ||
       prev_is_fullscreen != is_fullscreen);

  return need_update;
}

}  // namespace media
