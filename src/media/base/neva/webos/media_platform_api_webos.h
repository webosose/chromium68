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

#ifndef MEDIA_BASE_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_H_
#define MEDIA_BASE_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_H_

#include <mutex>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/synchronization/lock.h"
#include "media/base/neva/media_platform_api.h"
#include "media/base/neva/webos/lunaservice_client.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/jsoncpp/source/include/json/json.h"
#include "ui/gfx/geometry/rect.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class MEDIA_EXPORT MediaPlatformAPIWebOS : public MediaPlatformAPI {
 public:
  typedef base::Callback<void(const blink::WebRect&)> ActiveRegionCB;
  MediaPlatformAPIWebOS() {}
  static scoped_refptr<MediaPlatformAPIWebOS> Create(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      bool video,
      const std::string& app_id,
      const ActiveRegionCB& active_region_cb,
      const PipelineStatusCB& error_cb);

  // MediaPlatformAPI
  virtual void Initialize(const AudioDecoderConfig& audio_config,
                          const VideoDecoderConfig& video_config,
                          const PipelineStatusCB& init_cb) = 0;
  virtual void SetDisplayWindow(const gfx::Rect& rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) = 0;
  virtual void SetLoadCompletedCb(const LoadCompletedCB& loaded_cb) = 0;
  virtual bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
                    FeedType type) = 0;
  virtual uint64_t GetCurrentTime() = 0;
  virtual bool Seek(base::TimeDelta time) = 0;
  virtual void Suspend(SuspendReason reason) = 0;
  virtual void Resume(base::TimeDelta paused_time,
                      RestorePlaybackMode restore_playback_mode) = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual void SetPlaybackVolume(double volume) = 0;
  virtual bool AllowedFeedVideo() = 0;
  virtual bool AllowedFeedAudio() = 0;
  virtual void Finalize() = 0;
  virtual void SetKeySystem(const std::string& key_system) = 0;
  virtual bool IsEOSReceived() = 0;
  virtual void UpdateVideoConfig(const VideoDecoderConfig& video_config) {}
  // End of MediaPlatformAPI

  virtual void SwitchToAutoLayout() = 0;
  virtual void SetVisibility(bool visible) = 0;
  virtual bool Visibility() = 0;

 protected:
  virtual ~MediaPlatformAPIWebOS() {}

  DISALLOW_COPY_AND_ASSIGN(MediaPlatformAPIWebOS);
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_WEBOS_MEDIA_PLATFORM_API_WEBOS_H_
