// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_
#define MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "media/blink/neva/webos/webos_mediaclient.h"

namespace gfx {
class Rect;
}

namespace Json {
class Value;
}

#if UMS_INTERNAL_API_VERSION == 2
namespace ums {
struct audio_info_t;
struct video_info_t;
}
#else
namespace uMediaServer {
struct audio_info_t;
struct video_info_t;
}
#endif

namespace media {
class UMediaClientImpl;

// SystemMediaManager provide interface to platform media resource manager such
// as acb.
class SystemMediaManager {
 public:
  using ActiveRegionCB = WebOSMediaClient::ActiveRegionCB;

  enum class PlayState {
    kUnloaded,
    kLoaded,
    kPlaying,
    kPaused,
  };

  enum class AppState {
    kInit,
    kForeground,
    kBackground,
  };

  static std::unique_ptr<SystemMediaManager> Create(
      const base::WeakPtr<UMediaClientImpl>& umedia_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);

  virtual ~SystemMediaManager(){};

  virtual long Initialize(const bool is_video,
                          const std::string& app_id,
                          const ActiveRegionCB& active_region_cb) = 0;
  // |UMediaClientImpl| will call |UpdateHtmlMediaOption| before
  // |UMediaClientImpl| continue to Load if htmeMediaOption exists
  virtual void UpdateHtmlMediaOption(const Json::Value& option) = 0;
  // Set video out position in screen space by using in_rect in video space
  virtual bool SetDisplayWindow(const gfx::Rect& out_rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) = 0;
  // Set visibility of video
  virtual void SetVisibility(bool visible) = 0;
  // Get current visibility of video
  virtual bool GetVisibility() = 0;
  // Set the media audio focus
  virtual void SetAudioFocus() = 0;
  // Test the media has audio focus.
  virtual bool GetAudioFocus() = 0;
  // Switch to autolayout mode to prepare vtg
  virtual void SwitchToAutoLayout() = 0;
#if UMS_INTERNAL_API_VERSION == 2
  // Notify |UMediaClientImpl| has updated audio info
  virtual void AudioInfoUpdated(const struct ums::audio_info_t& audio_info) = 0;
  // Notify |UMediaClientImpl| has updated video info
  virtual void VideoInfoUpdated(const struct ums::video_info_t& videoInfo) = 0;
#else
  // Notify |UMediaClientImpl| has updated audio info
  virtual void AudioInfoUpdated(
      const struct uMediaServer::audio_info_t& audio_info) = 0;
  // Notify |UMediaClientImpl| has updated video info
  virtual void VideoInfoUpdated(
      const struct uMediaServer::video_info_t& videoInfo) = 0;
#endif
  // Notify app state is changed
  virtual void AppStateChanged(AppState s) = 0;
  // Notify play state is changed
  virtual void PlayStateChanged(PlayState s) = 0;
  // Notify audio mute is changed
  virtual void AudioMuteChanged(bool mute) = 0;
  // Send custom message to system media manager
  virtual bool SendCustomMessage(const std::string& message) = 0;
  // Eof recived
  virtual void EofReceived() = 0;
};

}  // namespace media
#endif  // MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_H_
