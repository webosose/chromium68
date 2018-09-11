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

#include "media/filters/neva/webos/audio_decoder_webos.h"

#include "base/callback_helpers.h"
#include "base/single_thread_task_runner.h"
#include "media/base/audio_buffer.h"
#include "media/base/audio_bus.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/audio_discard_helper.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/limits.h"
#include "media/base/sample_format.h"

namespace media {

AudioDecoderWebOS::AudioDecoderWebOS(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const scoped_refptr<MediaPlatformAPI>& media_platform_api)
    : EmptyBufferAudioDecoder(task_runner),
      media_platform_api_(media_platform_api) {
}

AudioDecoderWebOS::~AudioDecoderWebOS() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ != kUninitialized) {
    ResetTimestampState();
  }
}

std::string AudioDecoderWebOS::GetDisplayName() const {
  return "AudioDecoderWebOS";
}

void AudioDecoderWebOS::Initialize(const AudioDecoderConfig& config,
      CdmContext * cdm_context,
      const InitCB& init_cb,
      const OutputCB& output_cb,
      const WaitingForDecryptionKeyCB& waiting_for_decryption_key_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!config.is_encrypted());
  config_ = config;
  InitCB initialize_cb = BindToCurrentLoop(init_cb);

  if (!config.IsValidConfig() || !ConfigureDecoder() || !media_platform_api_) {
    initialize_cb.Run(false);
    return;
  }

  // Success!
  output_cb_ = BindToCurrentLoop(output_cb);
  state_ = kNormal;
  initialize_cb.Run(true);
}

bool AudioDecoderWebOS::FeedForPlatformMediaAudioDecoder(
    const scoped_refptr<DecoderBuffer>& buffer) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);

  if (!media_platform_api_)
    return false;
  return media_platform_api_->Feed(buffer, MediaPlatformAPI::Audio);
}

}  // namespace media
