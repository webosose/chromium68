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

#include "media/blink/neva/webos/umediaclient_gmp_impl.h"

#include <cmath>
#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "base/logging.h"
#include "base/single_thread_task_runner.h"
#include "base/strings/string_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/neva/webos/webos_media_pipeline_error.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define FUNC_LOG(x) DVLOG(x) << __func__
#define THIS_FUNC_LOG(x) DVLOG(x) << "[" << this << "] " << __func__

#define BIND_TO_RENDER_LOOP(function)                    \
  (DCHECK(media_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, weak_ptr_)))

namespace {
const char* kUdpUrl = "udp://";
const char* kRtpUrl = "rtp://";
const char* kRtspUrl = "rtsp://";
}

namespace media {

// static
std::unique_ptr<WebOSMediaClient> WebOSMediaClient::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  return std::make_unique<UMediaClientGmpImpl>(task_runner);
}

UMediaClientGmpImpl::UMediaClientGmpImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : duration_(0.),
      current_time_(0.),
      buffer_end_(0),
      buffer_end_at_last_didLoadingProgress_(0),
      video_(false),
      loaded_(false),
      preloaded_(false),
      load_started_(false),
      pending_unload_(false),
      is_reloading_(false),
      num_audio_tracks_(0),
      tile_no_(0),
      tile_count_(0),
      is_videowall_streaming_(false),
      is_local_source_(false),
      is_usb_file_(false),
      is_seeking_(false),
      is_suspended_(false),
      use_umsinfo_(false),
      use_pipeline_preload_(false),
      updated_source_info_(false),
      buffering_(false),
      requests_play_(false),
      requests_pause_(false),
      requests_videowall_play_(false),
      playback_rate_(0),
      playback_rate_on_eos_(0),
      playback_rate_on_paused_(0),
      volume_(1.0),
      media_task_runner_(task_runner),
      ls_client_(media::LunaServiceClient::PrivateBus),
      preload_(PreloadNone) {
  weak_ptr_ = AsWeakPtr();
}

UMediaClientGmpImpl::~UMediaClientGmpImpl() {
  THIS_FUNC_LOG(1);
  weak_ptr_.reset();
  if (!MediaId().empty() && loaded_) {
    uMediaServer::uMediaClient::unload();
  }
}

void UMediaClientGmpImpl::Load(
    bool video,
    bool reload,
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
    const EncryptedCB& encrypted_cb) {
  THIS_FUNC_LOG(1) << " url=" << url << " payload=" << payload;

  video_ = video;
  is_local_source_ = is_local_source;
  app_id_ = app_id;
  url_ = url;
  mime_type_ = mime_type;
  referrer_ = referrer;
  user_agent_ = user_agent;
  cookies_ = cookies;

  playback_state_cb_ = playback_state_cb;
  ended_cb_ = ended_cb;
  seek_cb_ = seek_cb;
  error_cb_ = error_cb;
  buffering_state_cb_ = buffering_state_cb;
  duration_change_cb_ = duration_change_cb;
  video_size_change_cb_ = video_size_change_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;
  update_ums_info_cb_ = update_ums_info_cb;
  focus_cb_ = focus_cb;

  updated_payload_ = UpdateMediaOption(payload, current_time_);

  using std::placeholders::_1;
  set_source_info_callback(
      std::bind(&UMediaClientGmpImpl::onSourceInfo, this, _1));
  set_video_info_callback(
      std::bind(&UMediaClientGmpImpl::onVideoInfo, this, _1));
  set_audio_info_callback(
      std::bind(&UMediaClientGmpImpl::onAudioInfo, this, _1));

  if (use_pipeline_preload_) {
    uMediaServer::uMediaClient::preload(url_.c_str(), kMedia,
                                        updated_payload_.c_str());
  } else {
    LoadInternal();
  }
}

void UMediaClientGmpImpl::Seek(base::TimeDelta time,
                               const media::PipelineStatusCB& seek_cb) {
  THIS_FUNC_LOG(1) << " time=" << time;
  current_time_ = time.InSecondsF();
  seek_cb_ = seek_cb;
  is_seeking_ = true;
  uMediaServer::uMediaClient::seek(time.InSecondsF() * 1000);
}

float UMediaClientGmpImpl::GetPlaybackRate() const {
  return playback_rate_;
}

void UMediaClientGmpImpl::SetPlaybackRate(float playback_rate) {
  if (MediaId().empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f) {
    requests_pause_ = true;
    playback_rate_on_paused_ = playback_rate_;
    uMediaServer::uMediaClient::pause();
  } else if (playback_rate_ == 0.0f) {
    requests_play_ = true;
    if (buffering_)
      DispatchBufferingEnd();
    if (playback_rate_on_paused_ != 1.0f || playback_rate != 1.0f)
      uMediaServer::uMediaClient::setPlayRate(playback_rate,
                                              CheckAudioOutput(playback_rate));
    if (use_pipeline_preload_ && !load_started_)
      LoadInternal();
    else
      uMediaServer::uMediaClient::play();
    playback_rate_on_eos_ = 0.0f;
  } else if (playback_rate != 0.0f && playback_rate != playback_rate_) {
    uMediaServer::uMediaClient::setPlayRate(playback_rate,
                                            CheckAudioOutput(playback_rate));
  } else if (requests_videowall_play_) {
    // forcing play for videowall gapless playback
    uMediaServer::uMediaClient::play();
  }

  playback_rate_ = playback_rate;
}

void UMediaClientGmpImpl::SetPlaybackVolume(double volume, bool forced) {
  if (!forced && (volume_ == volume))
    return;

  volume_ = volume;

  if (MediaId().empty() || !loaded_)
    return;

  int volume_level = (int)(volume * 100);
  int duration = 0;
  EaseType type = kEaseTypeLinear;

  uMediaServer::uMediaClient::setVolume(volume_level, duration, type);
}

void UMediaClientGmpImpl::Suspend() {
  THIS_FUNC_LOG(1) << " media_id=" << MediaId();
  is_suspended_ = true;
  uMediaServer::uMediaClient::notifyBackground();
  // force unload on suspend
  Unload();
}

void UMediaClientGmpImpl::Resume() {
  THIS_FUNC_LOG(1) << " media_id=" << MediaId();
  is_suspended_ = false;

  if (load_started_ && !loaded_) {
    uMediaServer::uMediaClient::notifyForeground();
    ReloadMediaResource();
    return;
  }

  if (use_pipeline_preload_ && !loaded_)
    return;

  uMediaServer::uMediaClient::notifyForeground();
}

void UMediaClientGmpImpl::SetPreload(Preload preload) {
  THIS_FUNC_LOG(1) << "  preload=" << preload;
#if UMS_INTERNAL_API_VERSION == 2
  /* RMP does not support preloading media, hence adding this temporary
   * workaround */
  preload = PreloadNone;
#endif
  if (use_pipeline_preload_ && !load_started_ && preload_ == PreloadMetaData &&
      preload == PreloadAuto) {
    LoadInternal();
    use_pipeline_preload_ = false;
  }
  preload_ = preload;
}

std::string UMediaClientGmpImpl::MediaId() {
  return uMediaServer::uMediaClient::media_id;
}

bool UMediaClientGmpImpl::SetDisplayWindow(const gfx::Rect& out_rect,
                                           const gfx::Rect& in_rect,
                                           bool fullscreen,
                                           bool forced) {
  using namespace uMediaServer;

  if (out_rect.IsEmpty())
    return false;

  if (!forced && previous_display_window_ == out_rect)
    return false;
  previous_display_window_ = out_rect;

  if (is_videowall_streaming_)
    return false;

  LOG(INFO) << __func__ << " out_rect=" << out_rect.ToString()
            << " in_rect=" << in_rect.ToString();

  if (fullscreen)
    switchToFullscreen();
  else if (in_rect.IsEmpty()) {
    uMediaClient::setDisplayWindow(rect_t(out_rect.x(), out_rect.y(),
                                          out_rect.width(), out_rect.height()));
  } else {
    uMediaClient::setDisplayWindow(
        rect_t(in_rect.x(), in_rect.y(), in_rect.width(), in_rect.height()),
        rect_t(out_rect.x(), out_rect.y(), out_rect.width(),
               out_rect.height()));
  }
  return true;
}

void UMediaClientGmpImpl::SetVisibility(bool visible) {}

bool UMediaClientGmpImpl::Visibility() {
  return true;
}

void UMediaClientGmpImpl::SetFocus() {}

bool UMediaClientGmpImpl::Focus() {
  return true;
}

bool UMediaClientGmpImpl::DidLoadingProgress() {
  if (!duration_)
    return false;

  int64_t current_buffer_end = buffer_end_;
  bool did_loading_progress =
      current_buffer_end != buffer_end_at_last_didLoadingProgress_;
  buffer_end_at_last_didLoadingProgress_ = current_buffer_end;

  return did_loading_progress;
}

bool UMediaClientGmpImpl::UsesIntrinsicSize() const {
  return !IsRequiredUMSInfo();
}

void UMediaClientGmpImpl::Unload() {
  LOG(INFO) << "[" << this << "] " << __func__
            << " media_id=" << MediaId().c_str()
            << " is_suspended=" << is_suspended_ << " loaded_=" << loaded_;
  if (!loaded_) {
    pending_unload_ = true;
    return;
  }
  uMediaServer::uMediaClient::unload();
  loaded_ = false;
}

bool UMediaClientGmpImpl::IsSupportedBackwardTrickPlay() {
  if (media_transport_type_ == "DLNA" || media_transport_type_ == "HLS-LG" ||
      media_transport_type_ == "USB" || media_transport_type_ == "DPS" ||
      media_transport_type_.empty())
    return true;
  return false;
}

bool UMediaClientGmpImpl::IsSupportedPreload() {
  // HLS, MPEG-DASH, URI is supported preload
  if (media_transport_type_.compare(0, 3, "HLS") == 0 ||
      media_transport_type_ == "MPEG-DASH" || media_transport_type_ == "URI" ||
      media_transport_type_ == "USB" || media_transport_type_.empty())
    return true;
  return false;
}

bool UMediaClientGmpImpl::CheckUseMediaPlayerManager(
    const std::string& media_option) {
  return false;
}

bool UMediaClientGmpImpl::onPlaying() {
  THIS_FUNC_LOG(1);
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchPlaying, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onPaused() {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchPaused, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onSeekDone() {
  if (!is_seeking_)
    return true;

  is_seeking_ = false;
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchSeekDone, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onEndOfStream() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchEndOfStream, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onLoadCompleted() {
  THIS_FUNC_LOG(1) << " loaded_=" << loaded_ << " media_id=" << MediaId();
  if (loaded_)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchLoadCompleted, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onPreloadCompleted() {
  THIS_FUNC_LOG(1) << " loaded_=" << loaded_;
  if (loaded_)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchPreloadCompleted, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onCurrentTime(int64_t currentTime) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchCurrentTime,
                            weak_ptr_, currentTime));
  return true;
}

#if UMS_INTERNAL_API_VERSION == 2
bool UMediaClientGmpImpl::onSourceInfo(
    const struct ums::source_info_t& sourceInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchSourceInfo, weak_ptr_,
                            sourceInfo));
  return true;
}

bool UMediaClientGmpImpl::onAudioInfo(
    const struct ums::audio_info_t& audioInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchAudioInfo, weak_ptr_,
                            audioInfo));
  return true;
}

bool UMediaClientGmpImpl::onVideoInfo(
    const struct ums::video_info_t& videoInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchVideoInfo, weak_ptr_,
                            videoInfo));
  return true;
}
#else
bool UMediaClientGmpImpl::onSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchSourceInfo, weak_ptr_,
                            sourceInfo));
  return true;
}

bool UMediaClientGmpImpl::onAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchAudioInfo, weak_ptr_,
                            audioInfo));
  return true;
}

bool UMediaClientGmpImpl::onVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchVideoInfo, weak_ptr_,
                            videoInfo));
  return true;
}
#endif

bool UMediaClientGmpImpl::onBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchBufferRange,
                            weak_ptr_, bufferRange));
  return true;
}

bool UMediaClientGmpImpl::onError(int64_t errorCode,
                                  const std::string& errorText) {
  // ignore buffer full/low error
  if (errorCode == SMP_BUFFER_FULL || errorCode == SMP_BUFFER_LOW)
    return true;

  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchError, weak_ptr_,
                            errorCode, errorText));
  return true;
}

bool UMediaClientGmpImpl::onExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchExternalSubtitleTrackInfo,
                 weak_ptr_, trackInfo));
  return true;
}

bool UMediaClientGmpImpl::onUserDefinedChanged(const char* message) {
  std::string msg = std::string(message);
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::DispatchUserDefinedChanged,
                            weak_ptr_, msg));
  return true;
}

bool UMediaClientGmpImpl::onBufferingStart() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchBufferingStart, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onBufferingEnd() {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchBufferingEnd, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onFocusChanged(bool) {
  media_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientGmpImpl::DispatchFocusChanged, weak_ptr_));
  return true;
}

bool UMediaClientGmpImpl::onActiveRegion(
    const uMediaServer::rect_t& active_region) {
  media_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientGmpImpl::dispatchActiveRegion,
                            weak_ptr_, active_region));
  return true;
}

void UMediaClientGmpImpl::DispatchPlaying() {
  THIS_FUNC_LOG(1);
  if (!requests_play_) {
    LOG(INFO) << __func__ << " requests_play_=" << requests_play_
              << " ignored!";
    return;
  }

  SetPlaybackVolume(volume_, true);
  requests_play_ = false;

  if (!playback_state_cb_.is_null())
    playback_state_cb_.Run(true);
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyPlaying));
}

void UMediaClientGmpImpl::DispatchPaused() {
  THIS_FUNC_LOG(1);
  requests_pause_ = false;
  playback_rate_on_paused_ = playback_rate_;
  playback_rate_ = 0.f;

  if (!playback_state_cb_.is_null())
    playback_state_cb_.Run(false);
}

void UMediaClientGmpImpl::DispatchSeekDone() {
  if (!seek_cb_.is_null())
    base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifySeekDone));
}

void UMediaClientGmpImpl::DispatchEndOfStream() {
  THIS_FUNC_LOG(1);

  playback_rate_on_eos_ = playback_rate_;
  if (duration_ > 0.)
    current_time_ = (playback_rate_ < 0) ? 0. : duration_;

  playback_rate_ = 0.0f;
  if (!ended_cb_.is_null())
    ended_cb_.Run();

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyEndOfStream));
}

void UMediaClientGmpImpl::DispatchLoadCompleted() {
  THIS_FUNC_LOG(1);
  if (loaded_) {
    LOG(INFO) << __func__ << "ignore duplicated loadCompleted event";
    return;
  }

  if (pending_unload_) {
    LOG(INFO) << __func__ << " has pending_unload";
    uMediaServer::uMediaClient::unload();
    pending_unload_ = false;
    return;
  }

  loaded_ = true;

  // to force for the client to call setDisplayWindow api
  if (is_reloading_ && !video_display_window_change_cb_.is_null()) {
    video_display_window_change_cb_.Run();
    is_reloading_ = false;
  }

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyLoadCompleted));

  if (IsNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    buffering_state_cb_.Run(UMediaClientGmpImpl::kHaveMetadata);
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kLoadCompleted);

  if (use_pipeline_preload_)
    uMediaServer::uMediaClient::play();
}

void UMediaClientGmpImpl::DispatchPreloadCompleted() {
  THIS_FUNC_LOG(1);

  preloaded_ = true;

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyPreloadCompleted));

  if (IsNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    buffering_state_cb_.Run(UMediaClientGmpImpl::kHaveMetadata);
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kPreloadCompleted);
}

void UMediaClientGmpImpl::DispatchCurrentTime(int64_t currentTime) {
  current_time_ = currentTime / 1000.;
  if (!seek_cb_.is_null())
    base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
}

#if UMS_INTERNAL_API_VERSION == 2
void UMediaClientGmpImpl::DispatchSourceInfo(
    const struct ums::source_info_t& sourceInfo) {
  THIS_FUNC_LOG(1);

  if (sourceInfo.programs.size() > 0) {
    if (sourceInfo.duration >= 0) {
      double updated_duration = sourceInfo.duration / 1000.;
      if (duration_ != updated_duration) {
        duration_ = updated_duration;
        if (!duration_change_cb_.is_null())
          duration_change_cb_.Run();
      }
    }

    has_video_ = (sourceInfo.programs[0].video_stream > 0) ? true : false;
    has_audio_ = (sourceInfo.programs[0].audio_stream > 0) ? true : false;
    num_audio_tracks_ = has_audio_ ? 1 : 0;

    if (IsInsufficientSourceInfo()) {
      has_audio_ = true;
      has_video_ = video_;
    }

    if (has_video_) {
      uint32_t video_stream = sourceInfo.programs[0].video_stream;
      if (video_stream > 0 && video_stream < sourceInfo.video_streams.size()) {
        ums::video_info_t video_info = sourceInfo.video_streams[video_stream];
        gfx::Size naturalVideoSize(video_info.width, video_info.height);

        if (natural_video_size_ != naturalVideoSize) {
          natural_video_size_ = naturalVideoSize;
          if (!video_size_change_cb_.is_null())
            video_size_change_cb_.Run();
        }
      }
    }
  }
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kHaveMetadata);

  if (!updated_source_info_) {
    if (loaded_)
      buffering_state_cb_.Run(UMediaClientGmpImpl::kLoadCompleted);
    else if (preloaded_)
      buffering_state_cb_.Run(UMediaClientGmpImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(sourceInfo));
}

void UMediaClientGmpImpl::DispatchAudioInfo(
    const struct ums::audio_info_t& audioInfo) {
  has_audio_ = true;
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(audioInfo));
}

void UMediaClientGmpImpl::DispatchVideoInfo(
    const struct ums::video_info_t& videoInfo) {
  THIS_FUNC_LOG(1);
  has_video_ = true;
  gfx::Size naturalVideoSize(videoInfo.width, videoInfo.height);

  if (natural_video_size_ != naturalVideoSize) {
    natural_video_size_ = naturalVideoSize;
    if (!video_size_change_cb_.is_null())
      video_size_change_cb_.Run();
  }

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(videoInfo));
  if (is_videowall_streaming_)
    setVideoWallDisplay(videoInfo);
}
#else  // UMS_INTERNAL_API_VERSION == 2
void UMediaClientGmpImpl::DispatchSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  THIS_FUNC_LOG(1);
  // Now numPrograms is always 1. if the value is 0, this case is invalid.
  if (sourceInfo.numPrograms > 0 && sourceInfo.programInfo.size()) {
    if (sourceInfo.programInfo[0].duration >= 0) {
      double updated_duration = sourceInfo.programInfo[0].duration / 1000.;
      if (duration_ != updated_duration) {
        duration_ = updated_duration;
        if (!duration_change_cb_.is_null())
          duration_change_cb_.Run();
      }
    }

    has_video_ = sourceInfo.programInfo[0].numVideoTracks ? true : false;
    has_audio_ = sourceInfo.programInfo[0].numAudioTracks ? true : false;
    num_audio_tracks_ =
        has_audio_ ? sourceInfo.programInfo[0].numAudioTracks : 0;

    if (IsInsufficientSourceInfo()) {
      has_audio_ = true;
      has_video_ = video_;
    }

    if (has_video_ && sourceInfo.programInfo[0].videoTrackInfo.size() &&
        sourceInfo.programInfo[0].videoTrackInfo[0].width &&
        sourceInfo.programInfo[0].videoTrackInfo[0].height) {
      gfx::Size naturalVideoSize(
          sourceInfo.programInfo[0].videoTrackInfo[0].width,
          sourceInfo.programInfo[0].videoTrackInfo[0].height);

      if (natural_video_size_ != naturalVideoSize) {
        natural_video_size_ = naturalVideoSize;
        if (!video_size_change_cb_.is_null())
          video_size_change_cb_.Run();
      }
    }
  }
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kHaveMetadata);

  if (!updated_source_info_) {
    if (loaded_)
      buffering_state_cb_.Run(UMediaClientGmpImpl::kLoadCompleted);
    else if (preloaded_)
      buffering_state_cb_.Run(UMediaClientGmpImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(sourceInfo));
}

void UMediaClientGmpImpl::DispatchAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  has_audio_ = true;
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(audioInfo));
}

void UMediaClientGmpImpl::DispatchVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  THIS_FUNC_LOG(1);
  has_video_ = true;
  gfx::Size naturalVideoSize(videoInfo.width, videoInfo.height);

  if (natural_video_size_ != naturalVideoSize) {
    natural_video_size_ = naturalVideoSize;
    if (!video_size_change_cb_.is_null())
      video_size_change_cb_.Run();
  }
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(videoInfo));
  if (is_videowall_streaming_)
    setVideoWallDisplay(videoInfo);
}
#endif

void UMediaClientGmpImpl::DispatchBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  buffer_end_ = bufferRange.endTime * 1000;
  if (duration_ > 0) {
    if (bufferRange.remainingTime == -1 || buffer_end_ > 1000 * duration_)
      buffer_end_ = 1000 * duration_;
  }
}

void UMediaClientGmpImpl::DispatchError(int64_t error_code,
                                        const std::string& error_text) {
  THIS_FUNC_LOG(x) << " media_id=" << MediaId() << " error_code=" << error_code
                   << " error_text=" << error_text;

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(error_code, error_text));

  media::PipelineStatus status = media::PIPELINE_OK;

  // refer following error code
  // http://collab.lge.com/main/display/NC50Platform/error
  if (SMP_STATUS_IS_100_GENERAL_ERROR(error_code)) {
    status = media::PIPELINE_ERROR_INITIALIZATION_FAILED;
    // Ignore 101(Command Not Supported) status
    if (error_code == SMP_COMMAND_NOT_SUPPORTED)
      status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_200_PLAYBACK_ERROR(error_code)) {
    // Playback related statuss (200 range)
    status = media::PIPELINE_ERROR_ABORT;

    // Ignore 200(Audio Codec Not Supported) status
    // when there is no audio track.
    if (error_code == SMP_AUDIO_CODEC_NOT_SUPPORTED &&
        (!has_audio_ || has_video_))
      status = media::PIPELINE_OK;
    // Ignore 201(Video Codec Not Supported) status
    // when there is no video track.
    if (error_code == SMP_VIDEO_CODEC_NOT_SUPPORTED && !has_video_)
      status = media::PIPELINE_OK;
    // Ignore 210(Unknown Subtitle) status
    if (error_code == SMP_UNKNOWN_SUBTITLE)
      status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_300_NETWORK_ERROR(error_code)) {
    // Network related statuss (300 range)
    status = media::PIPELINE_ERROR_NETWORK;
  } else if (SMP_STATUS_IS_400_SERVER_ERROR(error_code)) {
    // Server related statuss (400 range)
    status = media::PIPELINE_ERROR_NETWORK;
  } else if (SMP_STATUS_IS_500_DRM_ERROR(error_code)) {
    // DRM related statuss (500 range)
    status = media::PIPELINE_ERROR_DECRYPT;
  } else if (SMP_STATUS_IS_600_RM_ERROR(error_code)) {
    // resource is released by policy action
    if (error_code == SMP_RM_RELATED_ERROR) {
      status = media::DECODER_ERROR_RESOURCE_IS_RELEASED;
      // force paused playback state
      pause();
      DispatchPaused();
    }
    // allocation resources status
    if (error_code == SMP_RESOURCE_ALLOCATION_ERROR)
      status = media::PIPELINE_ERROR_ABORT;
  } else if (SMP_STATUS_IS_700_API_ERROR(error_code)) {
    // API functionality failure,
    // but not critical status for playback (700 range)
    status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_40000_STREAMING_ERROR(error_code)) {
    // Streaming Protocol related statuss (40000 ~ 49999 range)
    status = media::DEMUXER_ERROR_NO_SUPPORTED_STREAMS;
  }

  if (status != media::PIPELINE_OK && !error_cb_.is_null())
    base::ResetAndReturn(&error_cb_).Run(status);
}

void UMediaClientGmpImpl::DispatchExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(trackInfo));
}

void UMediaClientGmpImpl::DispatchUserDefinedChanged(
    const std::string& message) {
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(message));
}

void UMediaClientGmpImpl::DispatchBufferingStart() {
  THIS_FUNC_LOG(1);
  if (!current_time_ && requests_play_) {
    LOG(INFO) << __func__ << " current_time_=" << current_time_
              << " requests_play_=" << requests_play_;
    return;
  }

  buffering_ = true;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kWebOSBufferingStart);
}

void UMediaClientGmpImpl::DispatchBufferingEnd() {
  THIS_FUNC_LOG(1);
  buffering_ = false;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientGmpImpl::kWebOSBufferingEnd);
}

void UMediaClientGmpImpl::DispatchFocusChanged() {
  if (!focus_cb_.is_null()) {
    focus_cb_.Run();
  }
}

void UMediaClientGmpImpl::dispatchActiveRegion(
    const uMediaServer::rect_t& active_region) {
  if (!active_region_cb_.is_null()) {
    active_region_cb_.Run(blink::WebRect(active_region.x, active_region.y,
                                         active_region.w, active_region.h));
  }
}

#if UMS_INTERNAL_API_VERSION == 2
void UMediaClientGmpImpl::setVideoWallDisplay(
    const struct ums::video_info_t& videoInfo) {
  using namespace uMediaServer;
  THIS_FUNC_LOG(1);
  int gridCount = sqrt(tile_count_);
  int videoWidth = videoInfo.width / gridCount;
  int videoHeight = videoInfo.height / gridCount;
  int xCoordinate = ((tile_no_ - 1) % gridCount) * videoWidth;
  int yCoordinate = ((tile_no_ - 1) / gridCount) * videoHeight;

  uMediaClient::setDisplayWindow(
      rect_t(xCoordinate, yCoordinate, videoWidth, videoHeight),
      rect_t(previous_display_window_.x(), previous_display_window_.y(),
             previous_display_window_.width(),
             previous_display_window_.height()));
}
#else  // UMS_INTERNAL_API_VERSION == 2
void UMediaClientGmpImpl::setVideoWallDisplay(
    const struct uMediaServer::video_info_t& videoInfo) {
  using namespace uMediaServer;
  THIS_FUNC_LOG(1);
  int gridCount = sqrt(tile_count_);
  int videoWidth = videoInfo.width / gridCount;
  int videoHeight = videoInfo.height / gridCount;
  int xCoordinate = ((tile_no_ - 1) % gridCount) * videoWidth;
  int yCoordinate = ((tile_no_ - 1) / gridCount) * videoHeight;

  uMediaClient::setDisplayWindow(
      rect_t(xCoordinate, yCoordinate, videoWidth, videoHeight),
      rect_t(previous_display_window_.x, previous_display_window_.y,
             previous_display_window_.width, previous_display_window_.height));
}
#endif

std::string UMediaClientGmpImpl::MediaInfoToJson(
    const PlaybackNotification notification) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "playbackNotificationInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  switch (notification) {
    case NotifySeekDone:
      eventInfo["info"] = "seekDone";
      break;
    case NotifyPlaying:
      eventInfo["info"] = "playing";
      break;
    case NotifyPaused:
      eventInfo["info"] = "paused";
      break;
    case NotifyPreloadCompleted:
      eventInfo["info"] = "preloadCompleted";
      break;
    case NotifyLoadCompleted:
      eventInfo["info"] = "loadCompleted";
      break;
    case NotifyEndOfStream:
      eventInfo["info"] = "endOfStream";
      if (playback_rate_on_eos_ > 0.0f)
        eventInfo["direction"] = "forward";
      else if (playback_rate_on_eos_ < 0.0f)
        eventInfo["direction"] = "backward";
      break;
    default:
      return std::string("");
  }

  res = writer.write(eventInfo);

  return res;
}

#if UMS_INTERNAL_API_VERSION == 2
std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct ums::source_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value sourceInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "sourceInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  sourceInfo["container"] = value.container.c_str();
  sourceInfo["seekable"] = value.seekable;
  sourceInfo["numPrograms"] = value.programs.size();

  Json::Value programInfos(Json::arrayValue);
  for (int i = 0; i < value.programs.size(); i++) {
    Json::Value programInfo;
    programInfo["duration"] = static_cast<double>(value.duration);

    int numAudioTracks = 0;
    if (value.programs[i].audio_stream > 0 &&
        value.programs[i].audio_stream < value.audio_streams.size()) {
      numAudioTracks = 1;
    }
    programInfo["numAudioTracks"] = numAudioTracks;
    Json::Value audioTrackInfos(Json::arrayValue);
    for (int j = 0; j < numAudioTracks; j++) {
      Json::Value audioTrackInfo;
      int asi = value.programs[i].audio_stream;

      audioTrackInfo["codec"] = value.audio_streams[asi].codec.c_str();
      audioTrackInfo["bitRate"] = value.audio_streams[asi].bit_rate;
      audioTrackInfo["sampleRate"] = value.audio_streams[asi].sample_rate;

      audioTrackInfos.append(audioTrackInfo);
    }
    if (numAudioTracks)
      programInfo["audioTrackInfo"] = audioTrackInfos;

    int numVideoTracks = 0;
    if (value.programs[i].video_stream > 0 &&
        value.programs[i].video_stream < value.video_streams.size()) {
      numVideoTracks = 1;
    }

    Json::Value videoTrackInfos(Json::arrayValue);
    for (int j = 0; j < numVideoTracks; j++) {
      Json::Value videoTrackInfo;
      int vsi = value.programs[i].video_stream;

      float frame_rate = ((float)value.video_streams[vsi].frame_rate.num) /
                         ((float)value.video_streams[vsi].frame_rate.den);

      videoTrackInfo["codec"] = value.video_streams[vsi].codec.c_str();
      videoTrackInfo["width"] = value.video_streams[vsi].width;
      videoTrackInfo["height"] = value.video_streams[vsi].height;
      videoTrackInfo["frameRate"] = frame_rate;

      videoTrackInfo["bitRate"] = value.video_streams[vsi].bit_rate;

      videoTrackInfos.append(videoTrackInfo);
    }
    if (numVideoTracks)
      programInfo["videoTrackInfo"] = videoTrackInfos;

    programInfos.append(programInfo);
  }
  sourceInfo["programInfo"] = programInfos;

  eventInfo["info"] = sourceInfo;
  res = writer.write(eventInfo);

  if (previous_source_info_ == res)
    return std::string();

  previous_source_info_ = res;

  return res;
}

// refer to uMediaServer/include/public/dto_type.h
std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct ums::video_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value videoInfo;
  Json::Value frameRate;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "videoInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  videoInfo["width"] = value.width;
  videoInfo["height"] = value.height;
  frameRate["num"] = value.frame_rate.num;
  frameRate["den"] = value.frame_rate.den;
  videoInfo["frameRate"] = frameRate;
  videoInfo["codec"] = value.codec.c_str();
  videoInfo["bitRate"] = value.bit_rate;

  eventInfo["info"] = videoInfo;
  res = writer.write(eventInfo);

  LOG(INFO) << __func__ << " video_info=" << res;

  if (previous_video_info_ == res)
    return std::string();

  previous_video_info_ = res;

  return res;
}

std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct ums::audio_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value audioInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "audioInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  audioInfo["sampleRate"] = value.sample_rate;
  audioInfo["codec"] = value.codec.c_str();
  audioInfo["bitRate"] = value.bit_rate;

  eventInfo["info"] = audioInfo;
  res = writer.write(eventInfo);

  return res;
}
#else  // UMS_INTERNAL_API_VERSION == 2
std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct uMediaServer::source_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value sourceInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "sourceInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  sourceInfo["container"] = value.container.c_str();
  sourceInfo["seekable"] = value.seekable;
  sourceInfo["trickable"] = value.trickable;
  sourceInfo["numPrograms"] = value.numPrograms;

  Json::Value programInfos(Json::arrayValue);
  for (int i = 0; i < value.numPrograms; i++) {
    Json::Value programInfo;
    programInfo["duration"] =
        static_cast<double>(value.programInfo[i].duration);

    programInfo["numAudioTracks"] = value.programInfo[i].numAudioTracks;
    Json::Value audioTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numAudioTracks; j++) {
      Json::Value audioTrackInfo;
      audioTrackInfo["language"] =
          value.programInfo[i].audioTrackInfo[j].language.c_str();
      audioTrackInfo["codec"] =
          value.programInfo[i].audioTrackInfo[j].codec.c_str();
      audioTrackInfo["profile"] =
          value.programInfo[i].audioTrackInfo[j].profile.c_str();
      audioTrackInfo["level"] =
          value.programInfo[i].audioTrackInfo[j].level.c_str();
      audioTrackInfo["bitRate"] =
          value.programInfo[i].audioTrackInfo[j].bitRate;
      audioTrackInfo["sampleRate"] =
          value.programInfo[i].audioTrackInfo[j].sampleRate;
      audioTrackInfo["channels"] =
          value.programInfo[i].audioTrackInfo[j].channels;
      audioTrackInfos.append(audioTrackInfo);
    }
    if (value.programInfo[i].numAudioTracks)
      programInfo["audioTrackInfo"] = audioTrackInfos;

    programInfo["numVideoTracks"] = value.programInfo[i].numVideoTracks;
    Json::Value videoTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numVideoTracks; j++) {
      Json::Value videoTrackInfo;
      videoTrackInfo["angleNumber"] =
          value.programInfo[i].videoTrackInfo[j].angleNumber;
      videoTrackInfo["codec"] =
          value.programInfo[i].videoTrackInfo[j].codec.c_str();
      videoTrackInfo["profile"] =
          value.programInfo[i].videoTrackInfo[j].profile.c_str();
      videoTrackInfo["level"] =
          value.programInfo[i].videoTrackInfo[j].level.c_str();
      videoTrackInfo["width"] = value.programInfo[i].videoTrackInfo[j].width;
      videoTrackInfo["height"] = value.programInfo[i].videoTrackInfo[j].height;
      videoTrackInfo["aspectRatio"] =
          value.programInfo[i].videoTrackInfo[j].aspectRatio.c_str();
      videoTrackInfo["frameRate"] =
          value.programInfo[i].videoTrackInfo[j].frameRate;
      videoTrackInfo["bitRate"] =
          value.programInfo[i].videoTrackInfo[j].bitRate;
      videoTrackInfo["progressive"] =
          value.programInfo[i].videoTrackInfo[j].progressive;
      videoTrackInfos.append(videoTrackInfo);
    }
    if (value.programInfo[i].numVideoTracks)
      programInfo["videoTrackInfo"] = videoTrackInfos;

    programInfo["numSubtitleTracks"] = value.programInfo[i].numSubtitleTracks;
    Json::Value subtitleTrackInfos(Json::arrayValue);
    for (int j = 0; j < value.programInfo[i].numSubtitleTracks; j++) {
      Json::Value subtitleTrackInfo;
      subtitleTrackInfo["language"] =
          value.programInfo[i].subtitleTrackInfo[j].language.c_str();
      subtitleTrackInfos.append(subtitleTrackInfo);
    }
    if (value.programInfo[i].numSubtitleTracks)
      programInfo["subtitleTrackInfo"] = subtitleTrackInfos;

    programInfos.append(programInfo);
  }
  sourceInfo["programInfo"] = programInfos;

  eventInfo["info"] = sourceInfo;
  res = writer.write(eventInfo);

  if (previous_source_info_ == res)
    return std::string();

  previous_source_info_ = res;

  return res;
}

std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct uMediaServer::video_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value videoInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "videoInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  videoInfo["width"] = value.width;
  videoInfo["height"] = value.height;
  videoInfo["aspectRatio"] = value.aspectRatio.c_str();
  videoInfo["frameRate"] = value.frameRate;
  videoInfo["mode3D"] = value.mode3D.c_str();

  eventInfo["info"] = videoInfo;
  res = writer.write(eventInfo);

  if (previous_video_info_ == res)
    return std::string();

  previous_video_info_ = res;

  return res;
}

std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct uMediaServer::audio_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value audioInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "audioInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  audioInfo["sampleRate"] = value.sampleRate;
  audioInfo["channels"] = value.channels;

  eventInfo["info"] = audioInfo;
  res = writer.write(eventInfo);

  return res;
}
#endif

std::string UMediaClientGmpImpl::MediaInfoToJson(
    const struct uMediaServer::external_subtitle_track_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value externalSubtitleTrackInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "externalSubtitleTrackInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  externalSubtitleTrackInfo["uri"] = value.uri.c_str();
  externalSubtitleTrackInfo["hitEncoding"] = value.hitEncoding.c_str();
  externalSubtitleTrackInfo["numSubtitleTracks"] = value.numSubtitleTracks;

  Json::Value tracks(Json::arrayValue);
  for (int i = 0; i < value.numSubtitleTracks; i++) {
    Json::Value track;
    track["description"] = value.tracks[i].description.c_str();
    tracks.append(track);
  }
  externalSubtitleTrackInfo["tracks"] = tracks;

  eventInfo["info"] = externalSubtitleTrackInfo;
  res = writer.write(eventInfo);

  return res;
}

std::string UMediaClientGmpImpl::MediaInfoToJson(int64_t errorCode,
                                                 const std::string& errorText) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value error;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "error";
  eventInfo["mediaId"] = MediaId().c_str();

  error["errorCode"] = errorCode;
  error["errorText"] = errorText;

  eventInfo["info"] = error;
  res = writer.write(eventInfo);

  return res;
}

std::string UMediaClientGmpImpl::MediaInfoToJson(const std::string& message) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value userDefinedChanged;
  Json::Reader reader;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "userDefinedChanged";
  eventInfo["mediaId"] = MediaId().c_str();

  if (!reader.parse(message, userDefinedChanged)) {
    LOG(ERROR) << __func__ << " json_reader.parse error msg=" << message;
    return std::string();
  }

  eventInfo["info"] = userDefinedChanged;
  res = writer.write(eventInfo);

  if (previous_user_defined_changed_ == res)
    return std::string();

  previous_user_defined_changed_ = res;

  return res;
}

std::string UMediaClientGmpImpl::UpdateMediaOption(const std::string& option,
                                                   int64_t start) {
  Json::Reader reader;
  Json::FastWriter writer;
  Json::Value media_option;
  Json::Value http_header;
  std::string res;
  bool use_pipeline_preload = false;

  if (!option.empty()) {
    if (!reader.parse(option, media_option)) {
      LOG(ERROR) << __func__ << " json_reader.parse error option=" << option;
    } else if (media_option.isObject()) {
      if (media_option.isMember("htmlMediaOption")) {
        // Parse
        if (media_option["htmlMediaOption"].isMember("useUMSMediaInfo"))
          use_umsinfo_ =
              media_option["htmlMediaOption"]["useUMSMediaInfo"].asBool();
        if (media_option["htmlMediaOption"].isMember("usePipelinePreload"))
          use_pipeline_preload =
              media_option["htmlMediaOption"]["usePipelinePreload"].asBool();
        media_option.removeMember("htmlMediaOption");
      }
      if (media_option.isMember("mediaTransportType")) {
        media_transport_type_ = media_option["mediaTransportType"].asString();

        // support legacy spec for smartshare
        if (media_option["mediaTransportType"].asString() == "USB") {
          media_option["mediaTransportType"] = "URI";
          is_usb_file_ = true;
        }
      }
      if (media_option.isMember("videoWallInfo")) {
        tile_no_ = media_option["videoWallInfo"]["tileNo"].asUInt();
        tile_count_ = media_option["videoWallInfo"]["tileCount"].asUInt();
        is_videowall_streaming_ = true;
      }
    }
  }

  if (preload_ == PreloadMetaData) {
    use_pipeline_preload_ = true;
    if (!preloaded_ && is_usb_file_ && !use_pipeline_preload)
      use_pipeline_preload_ = false;
    if (!IsSupportedPreload())
      use_pipeline_preload_ = false;
  }

  http_header["referer"] = referrer_;
  http_header["userAgent"] = user_agent_;
  http_header["cookies"] = cookies_;
  media_option["option"]["transmission"]["httpHeader"] = http_header;
  media_option["option"]["bufferControl"]["userBufferCtrl"] = false;
  media_option["option"]["appId"] = app_id_;
  media_option["option"]["preload"] =
      (use_pipeline_preload_) ? "true" : "false";

  if (start)
    media_option["option"]["transmission"]["playTime"]["start"] = start;

  // check contents type
  if (!media_option.isMember("mediaTransportType")) {
    if (!mime_type_.empty()) {
      if (mime_type_ == "application/vnd.apple.mpegurl" ||
          mime_type_ == "application/mpegurl" ||
          mime_type_ == "application/x-mpegurl" ||
          mime_type_ == "application/vnd.apple.mpegurl.audio" ||
          mime_type_ == "audio/mpegurl" || mime_type_ == "audio/x-mpegurl")
        media_transport_type_ = "HLS";
      else if (mime_type_ == "application/dash+xml")
        media_transport_type_ = "MPEG-DASH";
      else if (mime_type_ == "application/vnd.ms-sstr+xml")
        media_transport_type_ = "MSIIS";
      else if (mime_type_ == "application/vnd.lge.gapless")
        media_transport_type_ = "GAPLESS";
    } else if (url_.find("m3u8") != std::string::npos) {
      media_transport_type_ = "HLS";
    }

    if (url_.find(kUdpUrl) != std::string::npos)
      media_transport_type_ = "UDP";
    else if (url_.find(kRtpUrl) != std::string::npos)
      media_transport_type_ = "RTP";
    else if (url_.find(kRtspUrl) != std::string::npos)
      media_transport_type_ = "RTSP";

    if (!media_transport_type_.empty())
      media_option["mediaTransportType"] = media_transport_type_;
  }

  if (use_umsinfo_ && media_transport_type_ == "GAPLESS")
    requests_videowall_play_ = true;

  if (media_option.empty())
    return std::string();

  res = writer.write(media_option);

  return res;
}

bool UMediaClientGmpImpl::IsRequiredUMSInfo() const {
  if (media_transport_type_ == "DLNA" || media_transport_type_ == "HLS-LG" ||
      media_transport_type_ == "USB" || media_transport_type_ == "MIRACAST" ||
      media_transport_type_ == "DPS" || use_umsinfo_)
    return true;
  return false;
}

bool UMediaClientGmpImpl::IsInsufficientSourceInfo() {
  if (media_transport_type_ == "HLS" || media_transport_type_ == "MSIIS" ||
      media_transport_type_ == "WIDEVINE" || media_transport_type_ == "DPS")
    return true;
  return false;
}

bool UMediaClientGmpImpl::IsAdaptiveStreaming() {
  if (media_transport_type_.compare(0, 3, "HLS") == 0 ||
      media_transport_type_ == "MSIIS" ||
      media_transport_type_ == "MPEG-DASH" ||
      media_transport_type_ == "WIDEVINE")
    return true;
  return false;
}

bool UMediaClientGmpImpl::IsNotSupportedSourceInfo() {
  if (media_transport_type_ == "MIRACAST" || media_transport_type_ == "RTP")
    return true;
  return false;
}

bool UMediaClientGmpImpl::Is2kVideoAndOver() {
  if (!video_)
    return false;

  if (natural_video_size_ == gfx::Size())
    return false;

  float macro_blocks_of_2k = 8704.0;
  float macro_blocks_of_video =
      ceilf((float)natural_video_size_.width() / 16.0) *
      ceilf((float)natural_video_size_.height() / 16.0);

  LOG(INFO) << __func__ << " macro_blocks_of_2k=" << macro_blocks_of_2k
            << " macro_blocks_of_video=" << macro_blocks_of_video;

  if (macro_blocks_of_video >= macro_blocks_of_2k)
    return true;

  return false;
}

bool UMediaClientGmpImpl::IsSupportedAudioOutputOnTrickPlaying() {
  if (media_transport_type_ == "DLNA" || media_transport_type_ == "HLS-LG" ||
      media_transport_type_ == "USB" || media_transport_type_ == "DPS")
    return true;
  return false;
}

gfx::Size UMediaClientGmpImpl::GetResoultionFromPAR(const std::string& par) {
  gfx::Size res(1, 1);

  size_t pos = par.find(":");
  if (pos == std::string::npos)
    return res;

  std::string w;
  std::string h;
  w.assign(par, 0, pos);
  h.assign(par, pos + 1, par.size() - pos - 1);
  return gfx::Size(std::stoi(w), std::stoi(h));
}

struct UMediaClientGmpImpl::Media3DInfo UMediaClientGmpImpl::GetMedia3DInfo(
    const std::string& media_3dinfo) {
  struct Media3DInfo res;
  res.type = "LR";

  if (media_3dinfo.find("RL") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "RL";
  } else if (media_3dinfo.find("LR") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "LR";
  } else if (media_3dinfo == "bottom_top") {
    res.pattern = "top_bottom";
    res.type = "RL";
  } else {
    res.pattern = media_3dinfo;
  }

  return res;
}

bool UMediaClientGmpImpl::CheckAudioOutput(float playback_rate) {
  if (playback_rate == 1.0f)
    return true;

  if (!IsSupportedAudioOutputOnTrickPlaying())
    return false;

  if (playback_rate != 0.5f && playback_rate != 2.0f)
    return false;

  if (playback_rate == 2.0f && Is2kVideoAndOver())
    return false;

  return true;
}

void UMediaClientGmpImpl::LoadInternal() {
  AudioStreamClass stream_type = kMedia;

  if (use_pipeline_preload_ && !is_suspended_)
    uMediaServer::uMediaClient::notifyForeground();

  if (media_transport_type_ == "GAPLESS")
    stream_type = kGapless;

  uMediaServer::uMediaClient::loadAsync(url_.c_str(), stream_type,
                                        updated_payload_.c_str());
  load_started_ = true;

  if (!use_pipeline_preload_ && !is_suspended_)
    uMediaServer::uMediaClient::notifyForeground();
}

void UMediaClientGmpImpl::ReloadMediaResource() {
  LOG(INFO) << "[" << this << "] " << __func__
            << " media_id was =" << MediaId().c_str() << " loaded_=" << loaded_;
  if (!load_started_ || loaded_ || pending_unload_) {
    pending_unload_ = false;
    return;
  }
  media_id = "";
  updated_payload_ = UpdateMediaOption(updated_payload_, current_time_);
  LOG(INFO) << __func__ << " loadAsync payload=" << updated_payload_;
  uMediaServer::uMediaClient::loadAsync(url_.c_str(), kMedia,
                                        updated_payload_.c_str());
  is_reloading_ = true;

  // if start time doesn't work, we need extra seek call
  uMediaServer::uMediaClient::seek(current_time_ * 1000);
}

}  // namespace media
