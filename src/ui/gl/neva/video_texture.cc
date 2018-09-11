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

#include "ui/gl/neva/video_texture.h"

//#define USE_TEXTURE_FLIP_V
namespace gfx {

static float mtxIdentity[16] = {
    1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1,
};

#if defined(USE_TEXTURE_FLIP_V)
static float mtxFlipV[16] = {
    1, 0, 0, 0, 0, -1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 1,
};
#endif

static void mtxMul(float out[16], const float a[16], const float b[16]);

// static
scoped_refptr<VideoTexture> VideoTexture::Create(uint32_t client_id) {
  return new VideoTexture(client_id);
}

// static
bool VideoTexture::IsSupported() {
  return VideoTextureManager::IsVideoTextureSupported();
}

VideoTexture::VideoTexture(uint32_t client_id)
    : client_id_(client_id),
      texture_id_(0),
      texture_width_(0),
      texture_height_(0),
      active_region_(0, 0, 1920, 1080) {
  VideoTextureManager::GetInstance()->Register(client_id_);
}

VideoTexture::~VideoTexture() {
  VideoTextureManager::GetInstance()->Unregister(client_id_);
}

void VideoTexture::SetActiveRegion(const gfx::Rect& active_region) {
  active_region_ = active_region;
}

void VideoTexture::SetFrameAvailableCallback(const base::Closure& callback) {
  VideoTextureManager::GetInstance()->SetFrameAvailableCallback(client_id_, callback);
}

void VideoTexture::UpdateTexImage() {
  unsigned int new_texture_id;
  VideoTextureManager::GetInstance()->Update(texture_id_, &new_texture_id,
                                             &texture_width_, &texture_height_);
  texture_id_ = new_texture_id;
}

void VideoTexture::GetTransformMatrix(float xform[16]) {
  float tx = 0.0f, ty = 0.0f, sx = 1.0f, sy = 1.0f;

  float cropWidth = active_region_.width();
  float cropHeight = active_region_.height();

  float cropLeft = active_region_.x();
#if defined(USE_TEXTURE_FLIP_V)
  float cropTop = texture_height_ - active_region_.bottom();
#else
  float cropTop = active_region_.y();
#endif

  if (cropWidth < texture_width_) {
    tx = cropLeft / texture_width_;
    sx = cropWidth / texture_width_;
  }
  if (cropHeight < texture_height_) {
    ty = cropTop / texture_height_;
    sy = cropHeight / texture_height_;
  }

  float crop[16] = {
      sx, 0, 0, 0, 0, sy, 0, 0, 0, 0, 1, 0, tx, ty, 0, 1,
  };

#if defined(USE_TEXTURE_FLIP_V)
  float mtxCrop[16];
  mtxMul(mtxCrop, crop, mtxIdentity);
  mtxMul(xform, mtxFlipV, mtxCrop);
#else
  mtxMul(xform, crop, mtxIdentity);
#endif
}

static void mtxMul(float out[16], const float a[16], const float b[16]) {
  out[0] = a[0] * b[0] + a[4] * b[1] + a[8] * b[2] + a[12] * b[3];
  out[1] = a[1] * b[0] + a[5] * b[1] + a[9] * b[2] + a[13] * b[3];
  out[2] = a[2] * b[0] + a[6] * b[1] + a[10] * b[2] + a[14] * b[3];
  out[3] = a[3] * b[0] + a[7] * b[1] + a[11] * b[2] + a[15] * b[3];
  out[4] = a[0] * b[4] + a[4] * b[5] + a[8] * b[6] + a[12] * b[7];
  out[5] = a[1] * b[4] + a[5] * b[5] + a[9] * b[6] + a[13] * b[7];
  out[6] = a[2] * b[4] + a[6] * b[5] + a[10] * b[6] + a[14] * b[7];
  out[7] = a[3] * b[4] + a[7] * b[5] + a[11] * b[6] + a[15] * b[7];
  out[8] = a[0] * b[8] + a[4] * b[9] + a[8] * b[10] + a[12] * b[11];
  out[9] = a[1] * b[8] + a[5] * b[9] + a[9] * b[10] + a[13] * b[11];
  out[10] = a[2] * b[8] + a[6] * b[9] + a[10] * b[10] + a[14] * b[11];
  out[11] = a[3] * b[8] + a[7] * b[9] + a[11] * b[10] + a[15] * b[11];
  out[12] = a[0] * b[12] + a[4] * b[13] + a[8] * b[14] + a[12] * b[15];
  out[13] = a[1] * b[12] + a[5] * b[13] + a[9] * b[14] + a[13] * b[15];
  out[14] = a[2] * b[12] + a[6] * b[13] + a[10] * b[14] + a[14] * b[15];
  out[15] = a[3] * b[12] + a[7] * b[13] + a[11] * b[14] + a[15] * b[15];
}

}  // namespace gfx
