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

#include "media/base/neva/webos/media_platform_api_webos_stub.h"

#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

// static
scoped_refptr<MediaPlatformAPIWebOS> MediaPlatformAPIWebOS::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const ActiveRegionCB& active_region_cb,
    const PipelineStatusCB& error_cb) {
  return base::MakeRefCounted<MediaPlatformAPIWebOSStub>();
}

MediaPlatformAPIWebOSStub::MediaPlatformAPIWebOSStub() {}

MediaPlatformAPIWebOSStub::~MediaPlatformAPIWebOSStub() {}

void MediaPlatformAPIWebOSStub::Initialize(
    const AudioDecoderConfig& audio_config,
    const VideoDecoderConfig& video_config,
    const PipelineStatusCB& init_cb) {}

void MediaPlatformAPIWebOSStub::SwitchToAutoLayout() {}

void MediaPlatformAPIWebOSStub::SetDisplayWindow(const gfx::Rect& rect,
                                                 const gfx::Rect& in_rect,
                                                 bool fullscreen) {}

bool MediaPlatformAPIWebOSStub::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                                     FeedType type) {
  return false;
}

uint64_t MediaPlatformAPIWebOSStub::GetCurrentTime() {
  return 0;
}

bool MediaPlatformAPIWebOSStub::Seek(base::TimeDelta time) {
  return false;
}

void MediaPlatformAPIWebOSStub::Suspend() {}

void MediaPlatformAPIWebOSStub::Resume(
    base::TimeDelta paused_time,
    RestorePlaybackMode restore_playback_mode) {}

void MediaPlatformAPIWebOSStub::SetPlaybackRate(float playback_rate) {}

void MediaPlatformAPIWebOSStub::SetPlaybackVolume(double volume) {}

bool MediaPlatformAPIWebOSStub::AllowedFeedVideo() {
  return false;
}

bool MediaPlatformAPIWebOSStub::AllowedFeedAudio() {
  return false;
}

void MediaPlatformAPIWebOSStub::Finalize() {}

void MediaPlatformAPIWebOSStub::SetKeySystem(const std::string& key_system) {}

bool MediaPlatformAPIWebOSStub::IsEOSReceived() {
  return false;
}

void MediaPlatformAPIWebOSStub::SetVisibility(bool visible) {}

bool MediaPlatformAPIWebOSStub::Visibility() {
  return true;
}

void MediaPlatformAPIWebOSStub::SetNaturalSize(const gfx::Size& size) {}

bool MediaPlatformAPIWebOSStub::Loaded() {
  return false;
}

std::string MediaPlatformAPIWebOSStub::GetMediaID() {
  return std::string();
}

bool MediaPlatformAPIWebOSStub::IsReleasedMediaResource() {
  return false;
}

}  // namespace media
