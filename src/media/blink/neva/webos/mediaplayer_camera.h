// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_CAMERA_H_
#define MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_CAMERA_H_

#include <cstdint>
#include <memory>
#include <string>

#include <uMediaClient.h>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "media/blink/neva/media_player_neva_interface.h"
#include "media/blink/webmediaplayer_util.h"

class GURL;

namespace media {

class MediaPlayerCamera : public media::MediaPlayerNeva,
                          public uMediaServer::uMediaClient,
                          public base::SupportsWeakPtr<MediaPlayerCamera> {
 public:
  enum ErrorCode {
    ImageDecodeError = 500,
    ImageDisplayError = 501,
  };

  MediaPlayerCamera(
      MediaPlayerNevaClient* client,
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  virtual ~MediaPlayerCamera();

  // media::MediaPlayerNeva implementation
  void Initialize(const bool is_video,
                  const double current_time,
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
  void Seek(const base::TimeDelta& time) override {}
  // Sets the player volume.
  void SetVolume(double volume) override {}
  // Sets the poster image.
  void SetPoster(const GURL& poster) override {}

  void SetRate(double rate) override;
  void SetPreload(Preload preload) override {}
  bool IsPreloadable(const std::string& content_media_option) override;
  bool HasVideo() override { return has_video_; }
  bool HasAudio() override { return has_audio_; }
  int NumAudioTracks() override { return 0; }
  bool SelectTrack(std::string& type, int32_t index) override { return false; }
  void SwitchToAutoLayout() override {}
  void SetDisplayWindow(const gfx::Rect& out_rect,
                        const gfx::Rect& in_rect,
                        bool full_screen,
                        bool forced) override;
  bool UsesIntrinsicSize() const override { return false; }
  std::string MediaId() const override { return media_id_; }
  void Suspend(SuspendReason reason) override {}
  void Resume() override {}
  bool IsRecoverableOnResume() override { return true; }
  bool HasAudioFocus() const override { return false; }
  void SetAudioFocus(bool focus) override {}
  bool HasVisibility() const override { return true; }
  void SetVisibility(bool) override {}
  bool RequireMediaResource() override { return false; }

  // uMediaServer::uMediaClient implementation
  bool onLoadCompleted() override;
  bool onUnloadCompleted() override;
  bool onPlaying() override;
  bool onPaused() override;
#if UMS_INTERNAL_API_VERSION == 2
  bool onVideoInfo(const struct ums::video_info_t&) override;
  bool onAudioInfo(const struct ums::audio_info_t&) override;
  bool onSourceInfo(const struct ums::source_info_t&) override;
#else
  bool onVideoInfo(const struct uMediaServer::video_info_t&) override;
  bool onAudioInfo(const struct uMediaServer::audio_info_t& audioInfo) override;
  bool onSourceInfo(const struct uMediaServer::source_info_t& srcInfo) override;
#endif
  bool onCurrentTime(int64_t currentTime) override;
  bool onError(long long errorCode, const std::string& errorText) override;
  bool onEndOfStream() override;
  bool onFileGenerated() override;
  bool onRecordInfo(const uMediaServer::record_info_t& recordInfo) override;
  bool onUserDefinedChanged(const char* message) override;

 private:
  friend class base::RefCountedThreadSafe<MediaPlayerCamera>;

  void DispatchLoadCompleted();
  void DispatchUnloadCompleted();
  void DispatchPlaying();
  void DispatchPaused();
  void DispatchUserDefinedChanged(const std::string& message);

#if UMS_INTERNAL_API_VERSION == 2
  void DispatchAudioInfo(const ums::audio_info_t& audioInfo);
  void DispatchVideoInfo(const ums::video_info_t& videoInfo);
  void DispatchSourceInfo(const ums::source_info_t& sourceInfo);
#else
  void DispatchAudioInfo(const uMediaServer::audio_info_t& audioInfo);
  void DispatchVideoInfo(const uMediaServer::video_info_t& videoInfo);
  void DispatchSourceInfo(const uMediaServer::source_info_t& sourceInfo);
#endif
  void DispatchCurrentTime(int64_t currentTime);
  void DispatchError(long long errorCode, const std::string& errorText);
  void DispatchEndOfStream();
  void DispatchFileGenerated();
  void DispatchRecordInfo(const uMediaServer::record_info_t& recordInfo);

  void UpdateCurrentTimeFired();

  MediaPlayerNevaClient* client_;

  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  bool has_audio_;
  bool has_video_;

  bool loaded_;

  std::string app_id_;
  GURL url_;
  std::string mime_type_;
  std::string camera_id_;
  std::string media_id_;

  base::RepeatingTimer time_update_timer_;
  base::TimeDelta last_time_update_time_;

  double current_time_;
  gfx::Rect display_window_;

  gfx::Size natural_video_size_;

  bool fullscreen_;
  gfx::Rect display_window_out_rect_;
  gfx::Rect display_window_in_rect_;

  base::WeakPtr<MediaPlayerCamera> weak_ptr_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlayerCamera);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBOS_MEDIAPLAYER_CAMERA_H_
