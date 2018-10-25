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

#include "media/filters/neva/webos/video_decoder_webos.h"

#include "base/bind.h"
#include "base/single_thread_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/video_decoder_config.h"

namespace media {

VideoDecoderWebOS::VideoDecoderWebOS(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<MediaPlatformAPI>& media_platform_api)
    : HoleFrameVideoDecoder(task_runner),
      media_platform_api_(media_platform_api) {
}

VideoDecoderWebOS::~VideoDecoderWebOS() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

std::string VideoDecoderWebOS::GetDisplayName() const {
  return "VideoDecoderWebOS";
}

void VideoDecoderWebOS::Initialize(const VideoDecoderConfig& config,
      bool low_delay,
      CdmContext * cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!config.is_encrypted());
  DCHECK(!output_cb.is_null());

  config_ = config;
  InitCB initialize_cb = BindToCurrentLoop(init_cb);

  // Our decoder couldn't decode encrypted video. But we still have
  // another chance for using this decoder for decrypted content,
  // when DecryptingDemuxerStream is created.
  // See DecoderSelector<StreamType>::InitializeDecoder().
  if (!config.IsValidConfig() || config.is_encrypted() ||
      !media_platform_api_) {
    initialize_cb.Run(false);
    return;
  }

  ConfigureDecoder(low_delay);

  output_cb_ = BindToCurrentLoop(output_cb);

  state_ = kNormal;
  initialize_cb.Run(true);
}

bool VideoDecoderWebOS::FeedForPlatformMediaVideoDecoder(
    const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);

  if (!media_platform_api_)
    return false;

  return media_platform_api_->Feed(buffer, MediaPlatformAPI::Video);
}

void VideoDecoderWebOS::ConfigureDecoder(bool low_delay) {
  if (media_platform_api_ && state_ != kUninitialized)
    media_platform_api_->UpdateVideoConfig(config_);
}

}  // namespace media
