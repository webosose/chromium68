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

#ifndef MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_UMS_H_
#define MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_UMS_H_

#include <map>
#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/renderer/render_frame_observer.h"
#include "media/blink/neva/media_player_neva_interface.h"
#include "media/blink/neva/webos/webos_mediaclient.h"
#include "url/gurl.h"

namespace blink {
class WebFrame;
}

namespace gfx {
class RectF;
}

namespace media {

class MediaPlayerUMS : public base::SupportsWeakPtr<MediaPlayerUMS>,
                       public media::MediaPlayerNeva {
 public:
  // Constructs a RendererMediaPlayerManager object for the |render_frame|.
  explicit MediaPlayerUMS(MediaPlayerNevaClient*,
                          const scoped_refptr<base::SingleThreadTaskRunner>&);
  virtual ~MediaPlayerUMS() override;

  // media::RendererMediaBuiltinPlayerManagerInterface implementation
  // Initializes a MediaPlayerAndroid object in browser process.
  void Initialize(const bool is_video,
                  const std::string& app_id,
                  const std::string& url,
                  const std::string& mime,
                  const std::string& referrer,
                  const std::string& user_agent,
                  const std::string& cookies,
                  const std::string& payload) override;

  // Starts the player.
  void Start() override;

  // Pauses the player.
  void Pause() override;

  // Performs seek on the player.
  void Seek(const base::TimeDelta& time) override;

  void SetRate(double rate) override;

  // Sets the player volume.
  void SetVolume(double volume) override;

  // Sets the poster image.
  void SetPoster(const GURL& poster) override;

  // bool IsSupportedBackwardTrickPlay() override;
  void SetPreload(
      Preload preload) override;  // TODO(wanchang): fix the type of preload
  bool IsPreloadable(const std::string& content_media_option) override;
  bool HasVideo() override;
  bool HasAudio() override;
  int NumAudioTracks() override;
  bool SelectTrack(std::string& type, int32_t index) override;
  // gfx::Size NaturalVideoSize() override;
  // double Duration() override;
  // double CurrentTime() override;
  void SwitchToAutoLayout() override;
  void SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullScreen,
                        bool forced = false) override;
  bool UsesIntrinsicSize() const override;
  std::string MediaId() const override;
  bool HasAudioFocus() const override;
  void SetAudioFocus(bool focus) override;
  bool HasVisibility() const override;
  void SetVisibility(bool) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  bool RequireMediaResource() override;
  // end of media::RendererMediaBuiltinPlayerManagerInterface
  //-----------------------------------------------------------------

 private:
  // umediaclient callbacks
  void OnPlaybackStateChanged(bool playing);
  void OnStreamEnded();
  void OnSeekDone(PipelineStatus status);
  void OnError(PipelineStatus error);
  void OnBufferingState(WebOSMediaClient::BufferingState buffering_state);
  void OnDurationChange();
  void OnVideoSizeChange();
  void OnVideoDisplayWindowChange();
  void OnAddAudioTrack(const std::string& id, const std::string& kind,
                       const std::string& language, bool enabled);
  void OnAddVideoTrack(const std::string& id, const std::string& kind,
                       const std::string& language, bool enabled);
  void UpdateUMSInfo(const std::string& detail);
  void OnAudioFocusChanged();
  void ActiveRegionChanged(const gfx::Rect& active_region);
  void OnWaitingForDecryptionKey();
  void OnEncryptedMediaInitData(const std::string& init_data_type,
                                const std::vector<uint8_t>& init_data);

  void OnTimeUpdateTimerFired();

  base::TimeDelta GetCurrentTime();

  std::unique_ptr<WebOSMediaClient> umedia_client_;
  MediaPlayerNevaClient* client_;
  bool paused_;
  base::TimeDelta paused_time_;
  double playback_rate_;
  bool is_suspended_;

  bool fullscreen_;
  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;
  gfx::Rect active_video_region_;
  bool active_video_region_changed_;

  bool is_video_offscreen_;

  base::RepeatingTimer time_update_timer_;

  base::TimeDelta last_time_update_timestamp_;

  base::MessageLoop* main_loop_;
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  DISALLOW_COPY_AND_ASSIGN(MediaPlayerUMS);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_UMS_H_
