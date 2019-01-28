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

#include "media/blink/neva/webos/umediaclient_impl.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/neva/webos/webos_media_pipeline_error.h"
#include "media/base/video_util.h"
#include "media/blink/neva/webos/media_util.h"
#include "media/blink/neva/webos/system_media_manager.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define FUNC_LOG(x) DVLOG(x) << __func__
#define THIS_FUNC_LOG(x) DVLOG(x) << "[" << this << "] " << __func__

namespace {
const char kUdpUrl[] = "udp://";
const char kRtpUrl[] = "rtp://";
const char kRtspUrl[] = "rtsp://";
}

namespace media {

// static
std::unique_ptr<WebOSMediaClient> WebOSMediaClient::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner) {
  return std::make_unique<UMediaClientImpl>(task_runner);
}

UMediaClientImpl::UMediaClientImpl(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : duration_(0.0f),
      current_time_(0.0f),
      buffer_end_(0.0f),
      buffer_end_at_last_didLoadingProgress_(0.0f),
      buffer_remaining_(0),
      start_date_(std::numeric_limits<double>::quiet_NaN()),
      video_(false),
      seekable_(true),
      ended_(false),
      has_video_(false),
      has_audio_(false),
      fullscreen_(false),
      num_audio_tracks_(0),
      tile_no_(0),
      tile_count_(0),
      is_videowall_streaming_(false),
      is_local_source_(false),
      is_seeking_(false),
      is_suspended_(false),
      use_umsinfo_(false),
      use_backward_trick_(false),
      use_pipeline_preload_(false),
      use_set_uri_(false),
      use_dass_control_(false),
      updated_source_info_(false),
      buffering_(false),
      requests_play_(false),
      requests_pause_(false),
      use_force_play_on_same_rate_(false),
      released_media_resource_(false),
      pixel_aspect_ratio_(1, 1),
      playback_rate_(0),
      playback_rate_on_eos_(0),
      playback_rate_on_paused_(1.0f),
      volume_(1.0),
      main_task_runner_(task_runner),
      ls_client_(media::LunaServiceClient::PrivateBus),
      system_media_manager_(
          SystemMediaManager::Create(AsWeakPtr(), task_runner)),
      preload_(PreloadNone),
      loading_state_(LOADING_STATE_NONE),
      pending_loading_action_(LOADING_ACTION_NONE) {
  // NOTE: AsWeakPtr() will create new valid WeakPtr even after it is
  // invalidated.
  // On our case, UMediaClientImpl will invalidate weakptr on its dtor
  // then will cleanup umediaclient message loop. (~UMediaClientImpl ->
  // ~SupportsWeakPtr -> ~uMediaClient -> ~WebOSEmdiaClient)
  // In this situation, calling AsWeakPtr() on umediaclient message loop will
  // cause this problem. To prevent this problem, we store on |weak_ptr_| and
  // use it on umediaclient message loop.
  weak_ptr_ = AsWeakPtr();
}

UMediaClientImpl::~UMediaClientImpl() {
  FUNC_LOG(1);
  weak_ptr_.reset();

  if (!MediaId().empty() && (is_loading() || is_loaded())) {
    system_media_manager_->PlayStateChanged(
        SystemMediaManager::PlayState::kUnloaded);
    UnloadInternal();
  }
}

void UMediaClientImpl::Load(bool video,
                            double current_time,
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
  FUNC_LOG(1) << " url=" << url << " payload=" << payload;

  video_ = video;
  current_time_ = current_time;
  is_local_source_ = is_local_source;
  app_id_ = app_id;
  url_ = url;
  mime_type_ = mime_type;
  referrer_ = referrer;
  user_agent_ = user_agent;
  cookies_ = cookies;
  buffering_state_have_meta_data_ = false;
  playback_state_cb_ = playback_state_cb;

  ended_cb_ = ended_cb;
  seek_cb_ = seek_cb;
  error_cb_ = error_cb;
  buffering_state_cb_ = buffering_state_cb;
  duration_change_cb_ = duration_change_cb;
  video_size_change_cb_ = video_size_change_cb;
  video_display_window_change_cb_ = video_display_window_change_cb;
  update_ums_info_cb_ = update_ums_info_cb;
  add_audio_track_cb_ = add_audio_track_cb;
  add_video_track_cb_ = add_video_track_cb;
  waiting_for_decryption_key_cb_ = waiting_for_decryption_key_cb;
  encrypted_cb_ = encrypted_cb;
  focus_cb_ = focus_cb;
  active_region_cb_ = active_region_cb;

  VLOG(2) << "currentTime: " << current_time_;
  updated_payload_ = UpdateMediaOption(payload, current_time_);

#if UMS_INTERNAL_API_VERSION == 2
  using std::placeholders::_1;
  set_source_info_callback(
      std::bind(&UMediaClientImpl::onSourceInfo, this, _1));
  set_video_info_callback(std::bind(&UMediaClientImpl::onVideoInfo, this, _1));
  set_audio_info_callback(std::bind(&UMediaClientImpl::onAudioInfo, this, _1));
#endif

  system_media_manager_->Initialize(video, app_id_, active_region_cb_);

  if (use_pipeline_preload_) {
    PreloadInternal(url_.c_str(), kMedia, updated_payload_.c_str());
  } else {
    LoadInternal();
  }
}

void UMediaClientImpl::Seek(base::TimeDelta time,
                            const media::PipelineStatusCB& seek_cb) {
  FUNC_LOG(1);

  ended_ = false;
  current_time_ = time.InSecondsF();
  seek_cb_ = seek_cb;
  is_seeking_ = true;
  uMediaServer::uMediaClient::seek(time.InSecondsF() * 1000);
}

float UMediaClientImpl::GetPlaybackRate() const {
  return playback_rate_;
}

void UMediaClientImpl::SetPlaybackRate(float playback_rate) {
  FUNC_LOG(1) << " rate=" << playback_rate;
  if (MediaId().empty()) {
    playback_rate_ = playback_rate;
    return;
  }

  if (playback_rate == 0.0f) {
    // * -> paused
    requests_pause_ = true;
    playback_rate_on_paused_ = playback_rate_;
    uMediaServer::uMediaClient::pause();
  } else if (playback_rate_ == 0.0f) {
    // paused -> play
    requests_play_ = true;
    if (buffering_)
      DispatchBufferingEnd();
    if (playback_rate_on_paused_ != 1.0f || playback_rate != 1.0f) {
      if (playback_rate_on_paused_ != playback_rate) {
        uMediaServer::uMediaClient::setPlayRate(
            playback_rate, CheckAudioOutput(playback_rate));
      }
    }
    if (use_pipeline_preload_ && !(is_loading() || is_loaded()))
      LoadInternal();
    else
      uMediaServer::uMediaClient::play();
    playback_rate_on_eos_ = 0.0f;
  } else if (playback_rate != 0.0f && playback_rate != playback_rate_) {
    // play -> play w/ different rate
    uMediaServer::uMediaClient::setPlayRate(playback_rate,
                                            CheckAudioOutput(playback_rate));
    if (playback_rate_ < 0.0f && playback_rate > 0.0f && ended_) {
      VLOG(1) << "reset ended_ as false. playbackrate: " << playback_rate
              << " / playbackrate_: " << playback_rate_;
      ended_ = false;
    }
  } else if (use_force_play_on_same_rate_) {
    // play -> play w/ same rate
    // forcing play for videowall gapless playback
    uMediaServer::uMediaClient::play();
  }

  playback_rate_ = playback_rate;
}

void UMediaClientImpl::SetPlaybackVolume(double volume, bool forced) {
  if (volume_ == volume && !forced)
    return;

  volume_ = volume;

  if (MediaId().empty() || loading_state_ != LOADING_STATE_LOADED)
    return;

  system_media_manager_->AudioMuteChanged(volume == 0);

  int volume_level = (int)(volume * 100);
  int duration = 0;
  EaseType type = kEaseTypeLinear;
  uMediaServer::uMediaClient::setVolume(volume_level, duration, type);
}

bool UMediaClientImpl::SelectTrack(std::string& type, int32_t index) {
  NOTIMPLEMENTED();
  return false;
}

void UMediaClientImpl::Suspend(SuspendReason reason) {
  FUNC_LOG(1) << " - MediaId: " << MediaId();
  is_suspended_ = true;

  bool force_unload = false;
#if defined(USE_GST_MEDIA)
  force_unload = true;
#endif

  if (force_unload) {
    released_media_resource_ = true;
    UnloadInternal();
    return;
  }

  if (use_pipeline_preload_ && !(is_loading() || is_loaded()))
    return;

  if (is_loading() || is_loaded()) {
    VLOG(1) << "call uMediaServer::uMediaClient::notifyBackground()";
    uMediaServer::uMediaClient::notifyBackground();
  }

  system_media_manager_->AppStateChanged(
      SystemMediaManager::AppState::kBackground);
}

void UMediaClientImpl::Resume() {
  FUNC_LOG(1) << " - MediaId: " << MediaId()
              << " loading_state=" << loading_state_
              << " IsReleasedMediaResource=" << IsReleasedMediaResource()
              << " current_time_=" << current_time_;
  is_suspended_ = false;

  if (IsReleasedMediaResource()) {
    if (is_loading()) {
      pending_loading_action_ = LOADING_ACTION_NONE;
      return;
    }
    updated_payload_ = UpdateMediaOption(updated_payload_, current_time_);
    LoadInternal();
    return;
  }

  if (use_pipeline_preload_ && !is_loaded())
    return;

  NotifyForeground();
  system_media_manager_->AppStateChanged(
      SystemMediaManager::AppState::kForeground);
}

bool UMediaClientImpl::IsRecoverableOnResume() {
  return true;
}

void UMediaClientImpl::SetPreload(Preload preload) {
  FUNC_LOG(1) << " app_id_=" << app_id_ << " preload=" << preload;
#if defined(USE_GST_MEDIA)
  // g-media-pipeline doesn't support preload
  preload = PreloadNone;
#endif

  if (use_pipeline_preload_ && !(is_loading() || is_loaded()) &&
      preload_ == PreloadMetaData && preload == PreloadAuto) {
    LoadInternal();
    use_pipeline_preload_ = false;
  }
  preload_ = preload;
}

bool UMediaClientImpl::IsPreloadable(const std::string& content_media_option) {
  UpdateMediaOption(content_media_option, 0.0f);
  return use_pipeline_preload_;
}

std::string UMediaClientImpl::MediaId() {
  return uMediaServer::uMediaClient::getMediaId();
}

bool UMediaClientImpl::SetDisplayWindow(const gfx::Rect& outRect,
                                        const gfx::Rect& inRect,
                                        bool fullscreen,
                                        bool forced) {
  if (!buffering_state_have_meta_data_) {
    FUNC_LOG(1) << " - setDisplayWindow is aborted";
    return false;
  }

  if (fullscreen_ != fullscreen)
    fullscreen_ = fullscreen;

  previous_display_window_ = outRect;
  return system_media_manager_->SetDisplayWindow(outRect, inRect, fullscreen);
}

void UMediaClientImpl::SetVisibility(bool visible) {
  system_media_manager_->SetVisibility(visible);
}

bool UMediaClientImpl::Visibility() {
  return system_media_manager_->GetVisibility();
}

void UMediaClientImpl::SetFocus() {
  return system_media_manager_->SetAudioFocus();
}

bool UMediaClientImpl::Focus() {
  return system_media_manager_->GetAudioFocus();
}

void UMediaClientImpl::SwitchToAutoLayout() {
  system_media_manager_->SwitchToAutoLayout();
}

bool UMediaClientImpl::DidLoadingProgress() {
  if (!duration_)
    return false;

  int64_t current_buffer_end = buffer_end_;
  bool did_loading_progress =
      current_buffer_end != buffer_end_at_last_didLoadingProgress_;
  buffer_end_at_last_didLoadingProgress_ = current_buffer_end;

  return did_loading_progress;
}

bool UMediaClientImpl::UsesIntrinsicSize() const {
  return true;
}

void UMediaClientImpl::Unload() {
  if (MediaId().empty())
    return;

  FUNC_LOG(1) << " - MediaId: " << MediaId();
  if (is_loaded()) {
    system_media_manager_->PlayStateChanged(
        SystemMediaManager::PlayState::kUnloaded);
    UnloadInternal();
    released_media_resource_ = true;
  } else if (is_loading()) {
    pending_loading_action_ = LOADING_ACTION_UNLOAD;
  }
}

bool UMediaClientImpl::IsSupportedBackwardTrickPlay() {
  NOTIMPLEMENTED();
  return false;
}

bool UMediaClientImpl::IsSupportedPreload() {
  return false;
}

bool UMediaClientImpl::CheckUseMediaPlayerManager(
    const std::string& mediaOption) {
#if defined(USE_GST_MEDIA)
  return false;
#else
  Json::Reader reader;
  Json::Value media_option;
  bool res = true;

  if (!mediaOption.empty()) {
    if (!reader.parse(mediaOption, media_option)) {
      VLOG(2) << "json_reader.parse error";
      return false;
    } else if (media_option.isObject()) {
      if (media_option.isMember("htmlMediaOption")) {
        if (media_option["htmlMediaOption"].isMember("useMediaPlayerManager"))
          res =
              media_option["htmlMediaOption"]["useMediaPlayerManager"].asBool();
      }
    }
  }

  return res;
#endif  // USE_GST_MEDIA
}
bool UMediaClientImpl::onPlaying() {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchPlaying, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchPlaying() {
  if (!requests_play_) {
    FUNC_LOG(1) << " ignored";
    return;
  }
  FUNC_LOG(2);

  // SystemMediaManager needs this call to connect audio sink right before
  // playing.
  SetPlaybackVolume(volume_, true);

  system_media_manager_->PlayStateChanged(
      SystemMediaManager::PlayState::kPlaying);

  requests_play_ = false;
  ended_ = false;

  if (!playback_state_cb_.is_null())
    playback_state_cb_.Run(true);

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyPlaying));

}

bool UMediaClientImpl::onPaused() {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchPaused, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchPaused() {
  if (!requests_pause_) {
    FUNC_LOG(1) << " ignored";
    return;
  }

  FUNC_LOG(2);
  requests_pause_ = false;

  system_media_manager_->PlayStateChanged(
      SystemMediaManager::PlayState::kPaused);

  if (!playback_state_cb_.is_null())
    playback_state_cb_.Run(false);
}

bool UMediaClientImpl::onSeekDone() {
  if (!is_seeking_)
    return true;

  is_seeking_ = false;
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchSeekDone, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchSeekDone() {
  FUNC_LOG(1);
  if (!seek_cb_.is_null())
    base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifySeekDone));
}

bool UMediaClientImpl::onEndOfStream() {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchEndOfStream, weak_ptr_,
                            (playback_rate_ > 0.0f)));
  return true;
}

void UMediaClientImpl::DispatchEndOfStream(bool isForward) {
  FUNC_LOG(1) << " isForward=" << isForward;
  bool ignore_eos = false;
  playback_rate_on_eos_ = playback_rate_;

  if (!ignore_eos) {
    ended_ = true;

    if (duration_ > 0.0f)
      current_time_ = (playback_rate_ < 0.0f) ? 0.0f : duration_;
    system_media_manager_->PlayStateChanged(
        SystemMediaManager::PlayState::kPaused);
    if (!ended_cb_.is_null())
      ended_cb_.Run();
  }

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyEndOfStream));
}

bool UMediaClientImpl::onLoadCompleted() {
  if (is_loaded())
    return true;

  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchLoadCompleted, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchLoadCompleted() {
  FUNC_LOG(1);

  if (is_loaded()) {
    VLOG(2) << "ignore duplicated loadCompleted event";
    return;
  }

  loading_state_ = LOADING_STATE_LOADED;
  released_media_resource_ = false;
  if (pending_loading_action_ == LOADING_ACTION_UNLOAD) {
    released_media_resource_ = true;
    UnloadInternal();
    pending_loading_action_ = LOADING_ACTION_NONE;
    return;
  }

  pending_loading_action_ = LOADING_ACTION_NONE;

  // TODO(neva): if we need to callback only when re-loading after unloaded
  // add flag to check.
  if (!video_display_window_change_cb_.is_null())
    video_display_window_change_cb_.Run();

  SetPlaybackVolume(volume_, true);
  system_media_manager_->PlayStateChanged(
      SystemMediaManager::SystemMediaManager::PlayState::kLoaded);

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(NotifyLoadCompleted));

  if (IsNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    system_media_manager_->SourceInfoUpdated(has_audio_, has_video_);
    if (!buffering_state_cb_.is_null()) {
      buffering_state_have_meta_data_ = true;
      buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
    }
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);

  if (use_pipeline_preload_) {
    if (playback_rate_ == 0.0f)
      requests_play_ = false;
    else
      uMediaServer::uMediaClient::play();
  }
}

bool UMediaClientImpl::onPreloadCompleted() {
  if (is_loaded())
    return true;

  // if don't use pipeline preload, skip preloadCompleted Event
  if (!use_pipeline_preload_)
    return true;

  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchPreloadCompleted, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchPreloadCompleted() {
  FUNC_LOG(1);

  loading_state_ = LOADING_STATE_PRELOADED;

  update_ums_info_cb_.Run(MediaInfoToJson(NotifyPreloadCompleted));

  if (IsNotSupportedSourceInfo()) {
    has_audio_ = true;
    has_video_ = video_;
    updated_source_info_ = true;
    system_media_manager_->SourceInfoUpdated(has_audio_, has_video_);
    if (!buffering_state_cb_.is_null()) {
      FUNC_LOG(2) << " buffering_state_have_meta_data_="
                  << buffering_state_have_meta_data_;
      buffering_state_have_meta_data_ = true;
      buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
    }
  }

  if (updated_source_info_ && !buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
}

bool UMediaClientImpl::onUnloadCompleted() {
  if (loading_state_ == LOADING_STATE_UNLOADED)
    return true;

  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchUnloadCompleted, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchUnloadCompleted() {
  FUNC_LOG(1);
  if (loading_state_ == LOADING_STATE_UNLOADED) {
    VLOG(1) << "ignore duplicated UnloadCompleted event";
    return;
  }
  media_id.clear();
  loading_state_ = LOADING_STATE_UNLOADED;
  if (pending_loading_action_ == LOADING_ACTION_LOAD) {
    LoadInternal();
    pending_loading_action_ = LOADING_ACTION_NONE;
  }
}

bool UMediaClientImpl::onCurrentTime(int64_t currentTime) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchCurrentTime, weak_ptr_,
                            currentTime));
  return true;
}

void UMediaClientImpl::DispatchCurrentTime(int64_t currentTime) {
  if (ended_) {
    VLOG(1) << "ignore currentTime event on ended - " << currentTime;
    return;
  }

  if (is_seeking_) {
    VLOG(1) << "ignore currentTime event on seeking - " << currentTime;
    return;
  }
  current_time_ = static_cast<double>(currentTime) / 1000.0;
}

bool UMediaClientImpl::onBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchBufferRange, weak_ptr_,
                            bufferRange));
  return true;
}

void UMediaClientImpl::DispatchBufferRange(
    const struct uMediaServer::buffer_range_t& bufferRange) {
  buffer_end_ = static_cast<double>(bufferRange.endTime);
  if (duration_ > 0) {
    if (bufferRange.remainingTime == -1 || buffer_end_ > duration_)
      buffer_end_ = duration_;
  }

  if (buffer_remaining_ != bufferRange.remainingTime &&
      !buffering_state_cb_.is_null()) {
    buffer_remaining_ = bufferRange.remainingTime;
    if (bufferRange.remainingTime > 0)
      buffering_state_cb_.Run(UMediaClientImpl::kWebOSNetworkStateLoading);
    else if (bufferRange.remainingTime == -1)
      buffering_state_cb_.Run(UMediaClientImpl::kWebOSNetworkStateLoaded);
  }
}

#if UMS_INTERNAL_API_VERSION == 2
bool UMediaClientImpl::onSourceInfo(
    const struct ums::source_info_t& sourceInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchSourceInfo, weak_ptr_, sourceInfo));
  return true;
}

bool UMediaClientImpl::onAudioInfo(const struct ums::audio_info_t& audioInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchAudioInfo, weak_ptr_, audioInfo));
  return true;
}

bool UMediaClientImpl::onVideoInfo(const struct ums::video_info_t& videoInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchVideoInfo, weak_ptr_, videoInfo));
  return true;
}

void UMediaClientImpl::DispatchSourceInfo(
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
  if (!buffering_state_cb_.is_null()) {
    buffering_state_have_meta_data_ = true;
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
  }

  if (!updated_source_info_) {
    if (is_loaded())
      buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);
    else if (loading_state_ == LOADING_STATE_PRELOADED)
      buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;
  system_media_manager_->SourceInfoUpdated(has_audio_, has_video_);

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(sourceInfo));
}

void UMediaClientImpl::DispatchAudioInfo(
    const struct ums::audio_info_t& audioInfo) {
  has_audio_ = true;
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(audioInfo));

  system_media_manager_->AudioInfoUpdated(audioInfo);
}

void UMediaClientImpl::DispatchVideoInfo(
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
  system_media_manager_->VideoInfoUpdated(videoInfo);
}
#else  // UMS_INTERNAL_API_VERSION == 2
bool UMediaClientImpl::onSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchSourceInfo, weak_ptr_, sourceInfo));
  return true;
}

void UMediaClientImpl::DispatchSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  FUNC_LOG(2);

  // Now numPrograms is always 1. if the value is 0, this case is invalid.
  if (sourceInfo.numPrograms > 0 && sourceInfo.programInfo.size()) {
    has_video_ = sourceInfo.programInfo[0].numVideoTracks ? true : false;
    has_audio_ = sourceInfo.programInfo[0].numAudioTracks ? true : false;
    num_audio_tracks_ =
        has_audio_ ? sourceInfo.programInfo[0].numAudioTracks : 0;
    seekable_ = sourceInfo.seekable;

    if (sourceInfo.startDate == -1)
      start_date_ = std::numeric_limits<double>::quiet_NaN();
    else
      start_date_ = static_cast<double>(sourceInfo.startDate);

    double duration = 0.0f;
    if (sourceInfo.programInfo[0].duration >= 0)
      duration =
          static_cast<double>(sourceInfo.programInfo[0].duration) / 1000.0;
    else
      duration = std::numeric_limits<double>::infinity();

    if (duration_ != duration) {
      duration_ = duration;
      if (!duration_change_cb_.is_null()) {
        VLOG(1) << "duration change - " << duration_
                << ", currentTime: " << current_time_;
        duration_change_cb_.Run();
      }
    }

    for (int i = 0; i < sourceInfo.programInfo[0].numAudioTracks; i++) {
      std::string id, kind;
      if (media_transport_type_ == "MPEG-DASH") {
        id = std::to_string(
            sourceInfo.programInfo[0].audioTrackInfo[i].adaptationSetId);
        kind = sourceInfo.programInfo[0].audioTrackInfo[i].role;
      } else {
#if USE_TRACKID
        if (sourceInfo.programInfo[0].audioTrackInfo[i].trackId > 0)
          id = std::to_string(
              sourceInfo.programInfo[0].audioTrackInfo[i].trackId);
        else
          id = std::to_string(sourceInfo.programInfo[0].audioTrackInfo[i].ctag);
#else
        // TODO(dongheun.kang): In Chrome 53, the id has been changed
        // to use the trackId. Therefore id can not be duplicated.
        // Temporarily change to use index.
        id = std::to_string(i + 1);
#endif
      }
      std::string language =
          sourceInfo.programInfo[0].audioTrackInfo[i].language.c_str();

      if (!add_audio_track_cb_.is_null())
        add_audio_track_cb_.Run(id, kind, language, false);
    }

    if (sourceInfo.programInfo[0].numVideoTracks > 0) {
      // Support only single track.
      std::string id, kind;
      if (media_transport_type_ == "MPEG-DASH") {
        id = std::to_string(
            sourceInfo.programInfo[0].videoTrackInfo[0].adaptationSetId);
        kind = sourceInfo.programInfo[0].videoTrackInfo[0].role;
      } else {
        if (sourceInfo.programInfo[0].videoTrackInfo[0].trackId > 0)
          id = std::to_string(
              sourceInfo.programInfo[0].videoTrackInfo[0].trackId);
        else
          id = std::to_string(sourceInfo.programInfo[0].videoTrackInfo[0].ctag);
      }

      if (!add_video_track_cb_.is_null())
        add_video_track_cb_.Run(id, kind, "", true);
    }

    if (IsInsufficientSourceInfo()) {
      has_audio_ = true;
      has_video_ = video_;
    }

    if (has_video_ && sourceInfo.programInfo[0].videoTrackInfo.size() &&
        sourceInfo.programInfo[0].videoTrackInfo[0].width &&
        sourceInfo.programInfo[0].videoTrackInfo[0].height) {
      gfx::Size videoSize =
          gfx::Size(sourceInfo.programInfo[0].videoTrackInfo[0].width,
                    sourceInfo.programInfo[0].videoTrackInfo[0].height);
      gfx::Size naturalVideoSize = media::GetNaturalSize(
          gfx::Size(sourceInfo.programInfo[0].videoTrackInfo[0].width,
                    sourceInfo.programInfo[0].videoTrackInfo[0].height),
          pixel_aspect_ratio_.width(), pixel_aspect_ratio_.height());

      if (natural_video_size_ != naturalVideoSize || video_size_ != videoSize) {
        video_size_ = videoSize;
        natural_video_size_ = naturalVideoSize;
        if (!video_size_change_cb_.is_null())
          video_size_change_cb_.Run();
      }
    }
  }
  if (!buffering_state_cb_.is_null()) {
    buffering_state_have_meta_data_ = true;
    buffering_state_cb_.Run(UMediaClientImpl::kHaveMetadata);
  }

  if (!updated_source_info_) {
    if (is_loaded())
      buffering_state_cb_.Run(UMediaClientImpl::kLoadCompleted);
    else if (loading_state_ == LOADING_STATE_PRELOADED)
      buffering_state_cb_.Run(UMediaClientImpl::kPreloadCompleted);
  }
  updated_source_info_ = true;
  system_media_manager_->SourceInfoUpdated(has_audio_, has_video_);

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(sourceInfo));
}

bool UMediaClientImpl::onAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchAudioInfo, weak_ptr_, audioInfo));
  return true;
}

void UMediaClientImpl::DispatchAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  has_audio_ = true;
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(audioInfo));

  system_media_manager_->AudioInfoUpdated(audioInfo);
}

bool UMediaClientImpl::onVideoInfo(
    const struct uMediaServer::video_info_t& video_info) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchVideoInfo, weak_ptr_, video_info));
  return true;
}

void UMediaClientImpl::DispatchVideoInfo(
    const struct uMediaServer::video_info_t& video_info) {
  FUNC_LOG(2);

  has_video_ = true;
  gfx::Size videoSize = gfx::Size(video_info.width, video_info.height);
  pixel_aspect_ratio_ = GetResolutionFromPAR(video_info.pixelAspectRatio);
  gfx::Size naturalVideoSize = media::GetNaturalSize(
      gfx::Size(video_info.width, video_info.height),
      pixel_aspect_ratio_.width(), pixel_aspect_ratio_.height());

  if (natural_video_size_ != naturalVideoSize || video_size_ != videoSize) {
    video_size_ = videoSize;
    natural_video_size_ = naturalVideoSize;
    if (!video_size_change_cb_.is_null())
      video_size_change_cb_.Run();
  }
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(video_info));

  system_media_manager_->VideoInfoUpdated(video_info);
}
#endif  // UMS_INTERNAL_API_VERSION == 2

bool UMediaClientImpl::onError(int64_t error_code,
                               const std::string& errorText) {
  // ignore buffer full/low error
  if (error_code == SMP_BUFFER_FULL || error_code == SMP_BUFFER_LOW)
    return true;

  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchError, weak_ptr_,
                            error_code, errorText));
  return true;
}

void UMediaClientImpl::DispatchError(int64_t error_code,
                                     const std::string& errorText) {
  FUNC_LOG(1) << " MediaId=" << MediaId() << " error_code=" << error_code
              << " msg=" << errorText;

  media::PipelineStatus status = CheckErrorCode(error_code);
  if (status != media::PIPELINE_OK) {
    if (is_loaded())
      UnloadInternal();
    system_media_manager_->PlayStateChanged(
        SystemMediaManager::PlayState::kUnloaded);
  }

  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(error_code, errorText));

  if (status != media::PIPELINE_OK && !error_cb_.is_null())
    base::ResetAndReturn(&error_cb_).Run(status);
}

bool UMediaClientImpl::onExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchExternalSubtitleTrackInfo,
                 weak_ptr_, trackInfo));
  return true;
}

void UMediaClientImpl::DispatchExternalSubtitleTrackInfo(
    const struct uMediaServer::external_subtitle_track_info_t& trackInfo) {
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(trackInfo));
}

media::PipelineStatus UMediaClientImpl::CheckErrorCode(int64_t error_code) {
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
        (!has_audio_ || has_video_ || video_))
      status = media::PIPELINE_OK;
    // Ignore 201(Video Codec Not Supported) status
    // when there is no video track.
    if (error_code == SMP_VIDEO_CODEC_NOT_SUPPORTED && !has_video_)
      status = media::PIPELINE_OK;
    // Ignore 210(Unknown Subtitle) status
    if (error_code == SMP_UNKNOWN_SUBTITLE)
      status = media::PIPELINE_OK;
    // 202(Media not found)
    if (error_code == SMP_MEDIA_NOT_FOUND) {
      status = media::PIPELINE_ERROR_INITIALIZATION_FAILED;
      if (media_transport_type_ == "MPEG-DASH")
        status = media::PIPELINE_ERROR_NETWORK;
    }
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
      released_media_resource_ = true;
#if defined(USE_GST_MEDIA)
      // force paused playback state
      pause();
      DispatchPaused();
#endif
    }
    // allocation resources status
    if (error_code == SMP_RESOURCE_ALLOCATION_ERROR ||
        error_code == SMP_DVR_RESOURCE_ALLOCATION_ERROR)
      status = media::PIPELINE_ERROR_ABORT;
  } else if (SMP_STATUS_IS_700_API_ERROR(error_code)) {
    // API functionality failure,
    // but not critical status for playback (700 range)
    if (error_code == SMP_SEEK_FAILURE) {
      if (is_seeking_)
        is_seeking_ = false;
      if (!seek_cb_.is_null())
        base::ResetAndReturn(&seek_cb_).Run(media::PIPELINE_OK);
    }
    status = media::PIPELINE_OK;
  } else if (SMP_STATUS_IS_1000_PIPELINE_ERROR(error_code)) {
    // uMS send when the pipeline crash occurred
    status = media::PIPELINE_ERROR_ABORT;
  } else if (SMP_STATUS_IS_40000_STREAMING_ERROR(error_code)) {
// Streaming Protocol related status (40000 ~ 49999 range)
    status = media::DEMUXER_ERROR_NO_SUPPORTED_STREAMS;
  }

  return status;
}

bool UMediaClientImpl::onUserDefinedChanged(const char* message) {
  if (!message)
    return true;

  std::string msg = std::string(message);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&UMediaClientImpl::DispatchUserDefinedChanged,
                            weak_ptr_, msg));
  return true;
}

void UMediaClientImpl::DispatchUserDefinedChanged(const std::string& message) {
  if (message.find("EOF") != std::string::npos &&
      message.find("pre_EOF") == std::string::npos) {
    system_media_manager_->EofReceived();
  }
  if (!update_ums_info_cb_.is_null())
    update_ums_info_cb_.Run(MediaInfoToJson(message));
}

bool UMediaClientImpl::onBufferingStart() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchBufferingStart, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchBufferingStart() {
  FUNC_LOG(1);
  if (current_time_ == 0.0f && requests_play_)
    return;

  buffering_ = true;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kWebOSBufferingStart);
}

bool UMediaClientImpl::onBufferingEnd() {
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&UMediaClientImpl::DispatchBufferingEnd, weak_ptr_));
  return true;
}

void UMediaClientImpl::DispatchBufferingEnd() {
  FUNC_LOG(1);
  buffering_ = false;
  if (!buffering_state_cb_.is_null())
    buffering_state_cb_.Run(UMediaClientImpl::kWebOSBufferingEnd);
}

#if UMS_INTERNAL_API_VERSION == 2
void UMediaClientImpl::setVideoWallDisplay(
    const struct ums::video_info_t& videoInfo) {
  using namespace uMediaServer;
  THIS_FUNC_LOG(1);
  int gridCount = sqrt(tile_count_);
  int videoWidth = videoInfo.width / gridCount;
  int videoHeight = videoInfo.height / gridCount;
  int xCoordinate = ((tile_no_ - 1) % gridCount) * videoWidth;
  int yCoordinate = ((tile_no_ - 1) / gridCount) * videoHeight;

  system_media_manager_->SetDisplayWindow(
      gfx::Rect(xCoordinate, yCoordinate, videoWidth, videoHeight),
      previous_display_window_, false);
}
#else  // UMS_INTERNAL_API_VERSION == 2
void UMediaClientImpl::setVideoWallDisplay(
    const struct uMediaServer::video_info_t& videoInfo) {
  using namespace uMediaServer;
  THIS_FUNC_LOG(1);
  int gridCount = sqrt(tile_count_);
  int videoWidth = videoInfo.width / gridCount;
  int videoHeight = videoInfo.height / gridCount;
  int xCoordinate = ((tile_no_ - 1) % gridCount) * videoWidth;
  int yCoordinate = ((tile_no_ - 1) / gridCount) * videoHeight;

  system_media_manager_->SetDisplayWindow(
      gfx::Rect(xCoordinate, yCoordinate, videoWidth, videoHeight),
      previous_display_window_, false);
}
#endif

std::string UMediaClientImpl::MediaInfoToJson(
    const PlaybackNotification notification) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::FastWriter writer;

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

  return writer.write(eventInfo);
}

#if UMS_INTERNAL_API_VERSION == 2
std::string UMediaClientImpl::MediaInfoToJson(
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
std::string UMediaClientImpl::MediaInfoToJson(
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

std::string UMediaClientImpl::MediaInfoToJson(
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
std::string UMediaClientImpl::MediaInfoToJson(
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
  sourceInfo["rotation"] = value.rotation;
  sourceInfo["numPrograms"] = value.numPrograms;

  Json::Value programInfos(Json::arrayValue);
  for (int i = 0; i < value.numPrograms; i++) {
    Json::Value programInfo;
    programInfo["duration"] =
        static_cast<int64_t>(value.programInfo[i].duration);
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

std::string UMediaClientImpl::MediaInfoToJson(
    const struct uMediaServer::video_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value videoInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "videoInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  videoInfo["aspectRatio"] = value.aspectRatio.c_str();
  videoInfo["width"] = value.width;
  videoInfo["height"] = value.height;
  videoInfo["frameRate"] = value.frameRate;
  videoInfo["mode3D"] = value.mode3D.c_str();

  eventInfo["info"] = videoInfo;
  res = writer.write(eventInfo);

  if (previous_video_info_ == res)
    return std::string();

  previous_video_info_ = res;

  return res;
}

std::string UMediaClientImpl::MediaInfoToJson(
    const struct uMediaServer::audio_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value audioInfo;
  Json::FastWriter writer;

  eventInfo["type"] = "audioInfo";
  eventInfo["mediaId"] = MediaId().c_str();

  audioInfo["sampleRate"] = value.sampleRate;
  audioInfo["channels"] = value.channels;

  eventInfo["info"] = audioInfo;

  return writer.write(eventInfo);
}
#endif  // UMS_INTERNAL_API_VERSION == 2

std::string UMediaClientImpl::MediaInfoToJson(
    const struct uMediaServer::external_subtitle_track_info_t& value) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value externalSubtitleTrackInfo;
  Json::FastWriter writer;

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
  return writer.write(eventInfo);
}

std::string UMediaClientImpl::MediaInfoToJson(int64_t errorCode,
                                              const std::string& errorText) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value error;
  Json::FastWriter writer;

  eventInfo["type"] = "error";
  eventInfo["mediaId"] = MediaId().c_str();

  error["errorCode"] = errorCode;
  error["errorText"] = errorText;

  eventInfo["info"] = error;

  return writer.write(eventInfo);
}

std::string UMediaClientImpl::MediaInfoToJson(const std::string& message) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value userDefinedChanged;
  Json::Reader reader;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "userDefinedChanged";
  eventInfo["mediaId"] = MediaId().c_str();

  if (message.empty()) {
    FUNC_LOG(1) << " - message is empty";
    return std::string();
  }

  if (!reader.parse(message, userDefinedChanged)) {
    FUNC_LOG(1) << " - json_reader.parse error: " << message;
    return std::string();
  }

  FUNC_LOG(1) << " - message: " << message;

  eventInfo["info"] = userDefinedChanged;
  res = writer.write(eventInfo);

  if (previous_user_defined_changed_ == res)
    return std::string();

  previous_user_defined_changed_ = res;

  return res;
}

std::string UMediaClientImpl::MediaInfoToJson(
    const struct uMediaServer::master_clock_info_t& masterClockInfo) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value setMasterClockResult;
  Json::FastWriter writer;

  eventInfo["type"] = "setMasterClockResult";
  eventInfo["mediaId"] = MediaId().c_str();

  setMasterClockResult["result"] = masterClockInfo.result;
  setMasterClockResult["port"] = masterClockInfo.port;
  setMasterClockResult["baseTime"] = masterClockInfo.baseTime;

  eventInfo["info"] = setMasterClockResult;
  return writer.write(eventInfo);
}

std::string UMediaClientImpl::MediaInfoToJson(
    const struct uMediaServer::slave_clock_info_t& slaveClockInfo) {
  if (!IsRequiredUMSInfo())
    return std::string();

  Json::Value eventInfo;
  Json::Value setSlaveClockResult;
  Json::FastWriter writer;

  eventInfo["type"] = "setSlaveClockResult";
  eventInfo["mediaId"] = MediaId().c_str();

  setSlaveClockResult["result"] = slaveClockInfo.result;

  eventInfo["info"] = setSlaveClockResult;
  return writer.write(eventInfo);
}

std::string UMediaClientImpl::UpdateMediaOption(const std::string& mediaOption,
                                                double start) {
  Json::Reader reader;
  Json::FastWriter writer;
  Json::Value media_option;
  Json::Value http_header;
  std::string res;
  bool use_pipeline_preload = false;

  if (!mediaOption.empty()) {
    if (!reader.parse(mediaOption, media_option)) {
      VLOG(2) << "json_reader.parse error";
    } else if (media_option.isObject()) {
      if (media_option.isMember("htmlMediaOption")) {
        // Parse
        if (media_option["htmlMediaOption"].isMember("useUMSMediaInfo"))
          use_umsinfo_ =
              media_option["htmlMediaOption"]["useUMSMediaInfo"].asBool();
        if (media_option["htmlMediaOption"].isMember("usePipelinePreload"))
          use_pipeline_preload =
              media_option["htmlMediaOption"]["usePipelinePreload"].asBool();
        if (media_option["htmlMediaOption"].isMember("useSetUri"))
          use_set_uri_ = media_option["htmlMediaOption"]["useSetUri"].asBool();

        system_media_manager_->UpdateHtmlMediaOption(
            media_option["htmlMediaOption"]);
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

  http_header["referer"] = referrer_;
  http_header["userAgent"] = user_agent_;
  http_header["cookies"] = cookies_;
  media_option["option"]["transmission"]["httpHeader"] = http_header;
  media_option["option"]["bufferControl"]["userBufferCtrl"] = false;
  media_option["option"]["appId"] = app_id_;
  media_option["option"]["preload"] =
      (use_pipeline_preload_) ? "true" : "false";

  if (!use_set_uri_)
    media_option["option"]["useSeekableRanges"] = true;

  if (start)
    media_option["option"]["transmission"]["playTime"]["start"] =
        static_cast<int64_t>(start * 1000.0);

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

    if (url_.find(".txt") != std::string::npos)
      media_transport_type_ = "GAPLESS";
    VLOG(2) << "media_transport_type_: " << media_transport_type_;

    if (!media_transport_type_.empty())
      media_option["mediaTransportType"] = media_transport_type_;
  }

  if (use_umsinfo_ && media_transport_type_ == "GAPLESS")
    use_force_play_on_same_rate_ = true;

  if (media_option.empty())
    return std::string();

  res = writer.write(media_option);

  VLOG(1) << "media_option: " << res;

  return res;
}

bool UMediaClientImpl::IsRequiredUMSInfo() {
  if (media_transport_type_ == "DLNA" || media_transport_type_ == "HLS-LG" ||
      media_transport_type_ == "USB" || media_transport_type_ == "MIRACAST" ||
      media_transport_type_ == "DPS" || use_umsinfo_)
    return true;
  return false;
}

bool UMediaClientImpl::IsInsufficientSourceInfo() {
  if (media_transport_type_ == "HLS" || media_transport_type_ == "MSIIS" ||
      media_transport_type_ == "WIDEVINE" || media_transport_type_ == "DPS")
    return true;
  return false;
}

bool UMediaClientImpl::IsAdaptiveStreaming() {
  if (media_transport_type_.compare(0, 3, "HLS") == 0 ||
      media_transport_type_ == "MIRACAST" || media_transport_type_ == "MSIIS" ||
      media_transport_type_ == "MPEG-DASH" ||
      media_transport_type_ == "WIDEVINE")
    return true;
  return false;
}

bool UMediaClientImpl::IsNotSupportedSourceInfo() {
  if (media_transport_type_ == "MIRACAST" || media_transport_type_ == "UDP" ||
      media_transport_type_ == "RTSP" || media_transport_type_ == "RTP")
    return true;
  return false;
}

bool UMediaClientImpl::IsAppName(const char* app_name) {
  return base::StartsWith(app_id_, app_name,
                          base::CompareCase::INSENSITIVE_ASCII);
}

bool UMediaClientImpl::Is2kVideoAndOver() {
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

bool UMediaClientImpl::IsSupportedAudioOutputOnTrickPlaying() {
  if (media_transport_type_ == "DLNA" || media_transport_type_ == "HLS-LG" ||
      media_transport_type_ == "USB" || media_transport_type_ == "DPS")
    return true;
  return false;
}

bool UMediaClientImpl::IsSupportedSeekableRanges() {
  if (media_transport_type_ == "MPEG-DASH" ||
      media_transport_type_ == "MSIIS") {
    return true;
  } else if (media_transport_type_.compare(0, 3, "HLS") == 0) {
    if (use_set_uri_)
      return false;
    return true;
  }
  return false;
}

void UMediaClientImpl::EnableSubtitle(bool enable) {
  if (MediaId().empty())
    return;

  Json::Value root;
  root["pipelineId"] = MediaId().c_str();

  Json::FastWriter writer;
  std::string parameter = writer.write(root);

  std::string uri = media::LunaServiceClient::GetServiceURI(
      media::LunaServiceClient::SUBTITLE,
      enable ? "enableSubtitle" : "disableSubtitle");

  ls_client_.callASync(uri, parameter);
}

bool UMediaClientImpl::CheckAudioOutput(float playback_rate) {
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

void UMediaClientImpl::LoadInternal() {
  AudioStreamClass stream_type = kMedia;
  if (use_pipeline_preload_ && !is_suspended_)
    NotifyForeground();

  FUNC_LOG(1) << " call uMediaServer::uMediaClient::loadAsync()";
  LoadAsyncInternal(url_.c_str(), stream_type, updated_payload_.c_str());

  if (!use_pipeline_preload_ && !is_suspended_)
    NotifyForeground();
}

bool UMediaClientImpl::UnloadInternal() {
  loading_state_ = LOADING_STATE_UNLOADING;
  return uMediaServer::uMediaClient::unload();
}

bool UMediaClientImpl::LoadAsyncInternal(const std::string& uri,
                                         AudioStreamClass audio_class,
                                         const std::string& media_payload) {
  FUNC_LOG(1) << " loading_state_ " << loading_state_ << " -> "
              << LOADING_STATE_LOADING;
  loading_state_ = LOADING_STATE_LOADING;
  return uMediaServer::uMediaClient::loadAsync(uri, audio_class, media_payload);
}

bool UMediaClientImpl::PreloadInternal(const std::string& uri,
                                       AudioStreamClass audio_class,
                                       const std::string& media_payload) {
  FUNC_LOG(1) << " loading_state_ " << loading_state_ << " -> "
              << LOADING_STATE_PRELOADING;
  loading_state_ = LOADING_STATE_PRELOADING;
  return uMediaServer::uMediaClient::preload(uri, audio_class, media_payload);
}

void UMediaClientImpl::NotifyForeground() {
  VLOG(1) << "call uMediaServer::uMediaClient::notifyForeground()";
  uMediaServer::uMediaClient::notifyForeground();
}

bool UMediaClientImpl::IsMpegDashContents() {
  return media_transport_type_ == "MPEG-DASH";
}

bool UMediaClientImpl::Send(const std::string& message) {
  return system_media_manager_->SendCustomMessage(message);
}

}  // namespace media
