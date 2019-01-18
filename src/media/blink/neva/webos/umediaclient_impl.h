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

#ifndef MEDIA_BLINK_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_
#define MEDIA_BLINK_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_

#include <memory>
#include <string>

#include <uMediaClient.h>
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/base/neva/webos/lunaservice_client.h"
#include "media/base/pipeline.h"
#include "media/base/ranges.h"
#include "media/blink/neva/webos/webos_mediaclient.h"
#include "media/blink/webmediaplayer_util.h"
#include "third_party/blink/public/platform/web_rect.h"

namespace base {
class Lock;
class SingleThreadTaskRunner;
}

namespace media {
class LunaServiceClient;
class SystemMediaManager;

class UMediaClientImpl : public WebOSMediaClient,
                         public uMediaServer::uMediaClient,
                         public base::SupportsWeakPtr<UMediaClientImpl> {
 public:
  UMediaClientImpl(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~UMediaClientImpl();

  // WebOSMediaClient implementations
  void Load(bool video,
            bool reload,  // TODO(wanchang): remove this
            bool is_local_source,
            const std::string& app_id,
            const std::string& url,
            const std::string& mime_type,
            const std::string& referrer,
            const std::string& user_agent,
            const std::string& cookies,
            const std::string& payload,
            const PlaybackStateCB& playback_state_cb,
            const base::Closure& ended_cb,
            const media::PipelineStatusCB& seek_cb,
            const media::PipelineStatusCB& error_cb,
            const BufferingStateCB& buffering_state_cb,
            const base::Closure& duration_change_cb,
            const base::Closure& video_size_change_cb,
            const base::Closure& video_display_window_change_cb,
            const AddAudioTrackCB& add_audio_track_cb,
            const AddVideoTrackCB& add_video_track_cb,
            const UpdateUMSInfoCB& update_ums_info_cb,
            const base::Closure& focus_cb,
            const ActiveRegionCB& active_region_cb,
            const base::Closure& waiting_for_decryption_key_cb,
            const EncryptedCB& encrypted_cb) override;
  void Seek(base::TimeDelta time,
            const media::PipelineStatusCB& seek_cb) override;
  float GetPlaybackRate() const override;
  void SetPlaybackRate(float playback_rate) override;
  double GetPlaybackVolume() const override { return volume_; }
  void SetPlaybackVolume(double volume, bool forced = false) override;
  bool SelectTrack(std::string& type, int32_t index) override;
  void Suspend(SuspendReason reason) override;
  void Resume() override;
  void SetPreload(Preload preload) override;
  bool IsPreloadable(const std::string& content_media_option) override;
  std::string MediaId() override;

  double GetDuration() const override { return duration_; }
  void SetDuration(double duration) override { duration_ = duration; }
  double GetCurrentTime() override { return current_time_; }
  void SetCurrentTime(double time) override { current_time_ = time; }

  double BufferEnd() const override { return buffer_end_; }
  bool HasAudio() override { return has_audio_; }
  bool HasVideo() override { return has_video_; }
  int GetNumAudioTracks() override { return num_audio_tracks_; }
  gfx::Size GetNaturalVideoSize() override { return natural_video_size_; }
  void SetNaturalVideoSize(const gfx::Size& size) override {
    natural_video_size_ = size;
  }

  bool SetDisplayWindow(const gfx::Rect&,
                        const gfx::Rect&,
                        bool fullscreen,
                        bool forced = false) override;
  void SetVisibility(bool visible) override;
  bool Visibility() override;
  void SetFocus() override;
  bool Focus() override;
  void SwitchToAutoLayout() override;
  bool DidLoadingProgress() override;
  bool UsesIntrinsicSize() const override;
  void Unload() override;
  bool IsSupportedBackwardTrickPlay() override;
  bool IsSupportedPreload() override;
  bool CheckUseMediaPlayerManager(const std::string& mediaOption) override;

  // uMediaServer::uMediaClient implementations
  bool onPlaying() override;
  bool onPaused() override;
  bool onSeekDone() override;
  bool onEndOfStream() override;
  bool onLoadCompleted() override;
  bool onPreloadCompleted() override;
  bool onUnloadCompleted() override;
  bool onCurrentTime(int64_t currentTime) override;
#if UMS_INTERNAL_API_VERSION == 2
  bool onAudioInfo(const struct ums::audio_info_t&);
  bool onVideoInfo(const struct ums::video_info_t&);
  bool onSourceInfo(const struct ums::source_info_t&);
#else
  bool onAudioInfo(const struct uMediaServer::audio_info_t&) override;
  bool onVideoInfo(const struct uMediaServer::video_info_t&) override;
  bool onSourceInfo(const struct uMediaServer::source_info_t&) override;
#endif
  bool onBufferRange(const struct uMediaServer::buffer_range_t&) override;
  bool onError(int64_t errorCode, const std::string& errorText) override;
  bool onExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&) override;
  bool onUserDefinedChanged(const char* message) override;
  bool onBufferingStart() override;
  bool onBufferingEnd() override;

  // dispatch event
  void DispatchPlaying();
  void DispatchPaused();
  void DispatchSeekDone();
  void DispatchEndOfStream(bool isForward);
  void DispatchLoadCompleted();
  void DispatchUnloadCompleted();
  void DispatchPreloadCompleted();
  void DispatchCurrentTime(int64_t currentTime);
  void DispatchBufferRange(const struct uMediaServer::buffer_range_t&);
#if UMS_INTERNAL_API_VERSION == 2
  void DispatchSourceInfo(const struct ums::source_info_t&);
  void DispatchAudioInfo(const struct ums::audio_info_t&);
  void DispatchVideoInfo(const struct ums::video_info_t&);
#else
  void DispatchSourceInfo(const struct uMediaServer::source_info_t&);
  void DispatchAudioInfo(const struct uMediaServer::audio_info_t&);
  void DispatchVideoInfo(const struct uMediaServer::video_info_t&);
#endif
  void DispatchError(int64_t errorCode, const std::string& errorText);
  void DispatchExternalSubtitleTrackInfo(
      const struct uMediaServer::external_subtitle_track_info_t&);
  void DispatchUserDefinedChanged(const std::string& message);
  void DispatchBufferingStart();
  void DispatchBufferingEnd();

  double GetStartDate() const { return start_date_; }
  bool IsSeekable() { return seekable_; }
  bool IsEnded() { return ended_; }
  bool IsReleasedMediaResource() { return released_media_resource_; }
  media::Ranges<base::TimeDelta> GetSeekableTimeRanges();
  void InitializeSeeking() { is_seeking_ = false; }
  void ResetEnded() { ended_ = false; }
  bool IsMpegDashContents();
  bool UseVideoWindowControl() { return use_video_window_control_; }
  bool Send(const std::string& message);

 private:
  typedef enum { PAUSED, PLAYING, SEEKING, SEEK_COMPLETED } PlayerState;

  typedef enum {
    LOADING_STATE_NONE,
    LOADING_STATE_PRELOADING,
    LOADING_STATE_PRELOADED,
    LOADING_STATE_LOADING,
    LOADING_STATE_LOADED,
    LOADING_STATE_UNLOADING,
    LOADING_STATE_UNLOADED
  } LoadingState;

  typedef enum {
    LOADING_ACTION_NONE,
    LOADING_ACTION_LOAD,
    LOADING_ACTION_UNLOAD
  } LoadingAction;

#if UMS_INTERNAL_API_VERSION == 2
  void setVideoWallDisplay(const struct ums::video_info_t&);
#else
  void setVideoWallDisplay(const struct uMediaServer::video_info_t&);
#endif
  std::string MediaInfoToJson(const PlaybackNotification);

#if UMS_INTERNAL_API_VERSION == 2
  std::string MediaInfoToJson(const struct ums::source_info_t&);
  std::string MediaInfoToJson(const struct ums::video_info_t&);
  std::string MediaInfoToJson(const struct ums::audio_info_t&);
#else
  std::string MediaInfoToJson(const struct uMediaServer::source_info_t&);
  std::string MediaInfoToJson(const struct uMediaServer::video_info_t&);
  std::string MediaInfoToJson(const struct uMediaServer::audio_info_t&);
#endif
  std::string MediaInfoToJson(
      const struct uMediaServer::external_subtitle_track_info_t&);
  std::string MediaInfoToJson(int64_t errorCode, const std::string& errorText);
  std::string MediaInfoToJson(const std::string& message);
  std::string MediaInfoToJson(const struct uMediaServer::master_clock_info_t&);
  std::string MediaInfoToJson(const struct uMediaServer::slave_clock_info_t&);

  std::string UpdateMediaOption(const std::string& mediaOption, double start);
  bool IsRequiredUMSInfo();
  bool IsInsufficientSourceInfo();
  bool IsAdaptiveStreaming();
  bool IsNotSupportedSourceInfo();
  bool IsAppName(const char* app_name);
  bool Is2kVideoAndOver();
  bool IsSupportedAudioOutputOnTrickPlaying();
  bool IsSupportedSeekableRanges();

  void SetMediaVideoData(const struct uMediaServer::video_info_t&,
                         bool forced = false);

  void EnableSubtitle(bool enable);

  bool CheckAudioOutput(float playback_rate);
  media::PipelineStatus CheckErrorCode(int64_t errorCode);
  void LoadInternal();
  bool UnloadInternal();
  bool LoadAsyncInternal(const std::string& uri,
                         AudioStreamClass audio_class,
                         const std::string& media_payload);
  bool PreloadInternal(const std::string& uri,
                       AudioStreamClass audio_class,
                       const std::string& media_payload);
  void NotifyForeground();
  inline bool is_loading() { return loading_state_ == LOADING_STATE_LOADING; }
  inline bool is_loaded() { return loading_state_ == LOADING_STATE_LOADED; }

  PlaybackStateCB playback_state_cb_;
  base::Closure ended_cb_;
  media::PipelineStatusCB seek_cb_;
  media::PipelineStatusCB error_cb_;
  base::Closure duration_change_cb_;
  base::Closure video_size_change_cb_;
  base::Closure video_display_window_change_cb_;
  AddAudioTrackCB add_audio_track_cb_;
  AddVideoTrackCB add_video_track_cb_;
  UpdateUMSInfoCB update_ums_info_cb_;
  BufferingStateCB buffering_state_cb_;
  base::Closure focus_cb_;
  bool buffering_state_have_meta_data_;
  ActiveRegionCB active_region_cb_;
  base::Closure waiting_for_decryption_key_cb_;
  EncryptedCB encrypted_cb_;
  double duration_;
  double current_time_;
  double buffer_end_;
  double buffer_end_at_last_didLoadingProgress_;
  int64_t buffer_remaining_;
  double start_date_;
  bool video_;
  bool seekable_;
  bool ended_;
  bool has_video_;
  bool has_audio_;
  bool fullscreen_;
  int num_audio_tracks_;
  int tile_no_;
  int tile_count_;
  bool is_videowall_streaming_;
  bool is_local_source_;
  bool is_usb_file_;
  bool is_seeking_;
  bool is_suspended_;
  bool use_umsinfo_;
  bool use_backward_trick_;
  bool use_pipeline_preload_;
  bool use_set_uri_;
  bool use_video_window_control_;
  bool use_dass_control_;
  bool updated_source_info_;
  bool buffering_;
  bool requests_play_;
  bool requests_pause_;
  bool use_force_play_on_same_rate_;
  bool released_media_resource_;
  std::string media_transport_type_;
  gfx::Size natural_video_size_;
  gfx::Size video_size_;
  gfx::Size pixel_aspect_ratio_;
  float playback_rate_;
  float playback_rate_on_eos_;
  float playback_rate_on_paused_;
  double volume_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;
  media::LunaServiceClient ls_client_;
  std::string app_id_;
  std::string url_;
  std::string mime_type_;
  std::string referrer_;
  std::string user_agent_;
  std::string cookies_;
  std::string previous_source_info_;
  std::string previous_video_info_;
  std::string previous_user_defined_changed_;
  std::string previous_media_video_data_;
  std::string updated_payload_;
  gfx::Rect previous_display_window_;
  std::unique_ptr<SystemMediaManager> system_media_manager_;
  long current_play_state_;

  mutable base::Lock lock_;
  media::Ranges<base::TimeDelta> seekable_ranges_;

  Preload preload_;

  bool visibility_;

  PlayerState player_state_;    // paused, playing, seeking, seek_completed
  LoadingState loading_state_;  // unloaded, loading, loaded, unloading
  LoadingAction pending_loading_action_;

  base::WeakPtr<UMediaClientImpl> weak_ptr_;

  DISALLOW_COPY_AND_ASSIGN(UMediaClientImpl);
};

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_WEBOS_UMEDIACLIENT_IMPL_H_
