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

#ifndef UI_GL_NEVA_VIDEO_TEXTURE_H_
#define UI_GL_NEVA_VIDEO_TEXTURE_H_

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/timer/timer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gl/gl_export.h"
#include "ui/gl/neva/video_texture_manager.h"

namespace gfx {

class GL_EXPORT VideoTexture : public base::RefCountedThreadSafe<VideoTexture> {
 public:
  static scoped_refptr<VideoTexture> Create(uint32_t client_id);

  static bool IsSupported();

  void SetActiveRegion(const gfx::Rect& active_region);

  void SetFrameAvailableCallback(const base::Closure& callback);

  // Update the texture image to the most recent frame from the image stream.
  void UpdateTexImage();

  // Retrieve the 4x4 texture coordinate transform matrix associated with the
  // texture image set by the most recent call to updateTexImage.
  void GetTransformMatrix(float xform[16]);

  unsigned int texture_id() const {
    return texture_id_;
  }

 protected:
  explicit VideoTexture(uint32_t client_id);

 private:
  friend class base::RefCountedThreadSafe<VideoTexture>;
  ~VideoTexture();

  uint32_t client_id_;

  unsigned int texture_id_;
  int texture_width_;
  int texture_height_;

  gfx::Rect active_region_;

  DISALLOW_COPY_AND_ASSIGN(VideoTexture);
};
}

#endif  // UI_GL_NEVA_VIDEO_TEXTURE_H_
