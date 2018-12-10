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

#include "media/base/neva/webos/media_platform_api_webos_gmp.h"

#pragma GCC optimize("rtti")
#include <gmp/MediaPlayerClient.h>
#pragma GCC reset_options

#include "base/callback_helpers.h"
#include "base/command_line.h"
#include "content/public/common/content_switches.h"
#include "media/base/bind_to_current_loop.h"

#define FUNC_LOG(x) DVLOG(x) << __func__
#define FUNC_THIS_LOG(x) DVLOG(x) << "[" << this << "] " << __func__

namespace media {

namespace {
const size_t kMaxPendingFeedSize = 15 * 1024 * 1024;  // 15MB

GMP_VIDEO_CODEC video_codec[] = {
    GMP_VIDEO_CODEC_NONE,  GMP_VIDEO_CODEC_H264,  GMP_VIDEO_CODEC_VC1,
    GMP_VIDEO_CODEC_MPEG2, GMP_VIDEO_CODEC_MPEG4, GMP_VIDEO_CODEC_THEORA,
    GMP_VIDEO_CODEC_VP8,   GMP_VIDEO_CODEC_VP9,   GMP_VIDEO_CODEC_H265,
};

GMP_AUDIO_CODEC audio_codec[] = {
    GMP_AUDIO_CODEC_NONE,      GMP_AUDIO_CODEC_AAC,
    GMP_AUDIO_CODEC_MP3,       GMP_AUDIO_CODEC_PCM,
    GMP_AUDIO_CODEC_VORBIS,    GMP_AUDIO_CODEC_FLAC,
    GMP_AUDIO_CODEC_AMR_NB,    GMP_AUDIO_CODEC_AMR_WB,
    GMP_AUDIO_CODEC_PCM_MULAW, GMP_AUDIO_CODEC_GSM_MS,
    GMP_AUDIO_CODEC_PCM_S16BE, GMP_AUDIO_CODEC_PCM_S24BE,
    GMP_AUDIO_CODEC_OPUS,      GMP_AUDIO_CODEC_EAC3,
    GMP_AUDIO_CODEC_PCM_ALAW,  GMP_AUDIO_CODEC_ALAC,
    GMP_AUDIO_CODEC_AC3,
};

std::set<MediaPlatformAPIWebOSGmp*> g_media_apis_set_;
std::mutex g_media_apis_set_lock_;

const char* GmpNotifyTypeToString(gint type) {
#define STRINGIFY_NOTIFY_TYPE_CASE(type) \
  case type:                             \
    return #type
  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_LOAD_COMPLETED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_UNLOAD_COMPLETED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SOURCE_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_END_OF_STREAM);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_CURRENT_TIME);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SEEK_DONE);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_PLAYING);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_PAUSED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_NEED_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_ENOUGH_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_SEEK_DATA);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_ERROR);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_VIDEO_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_AUDIO_INFO);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_FULL);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_NEED);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFER_RANGE);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFERING_START);
    STRINGIFY_NOTIFY_TYPE_CASE(NOTIFY_BUFFERING_END);
    default:
      return "null";
  }
  return "null";
}
}

// static
scoped_refptr<MediaPlatformAPIWebOS> MediaPlatformAPIWebOS::Create(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const ActiveRegionCB& active_region_cb,
    const PipelineStatusCB& error_cb) {
  return base::MakeRefCounted<MediaPlatformAPIWebOSGmp>(task_runner, video,
                                                        app_id, error_cb);
}

void MediaPlatformAPIWebOSGmp::Callback(const gint type,
                                        const gint64 num_value,
                                        const gchar* str_value,
                                        void* user_data) {
  FUNC_LOG(1) << " type=" << GmpNotifyTypeToString(type)
              << " num_value=" << num_value << " str_value=" << str_value
              << " data=" << user_data;
  MediaPlatformAPIWebOSGmp* that =
      static_cast<MediaPlatformAPIWebOSGmp*>(user_data);
  if (that && that->is_finalized_)
    return;

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    if (!that || g_media_apis_set_.find(that) == g_media_apis_set_.end()) {
      LOG(ERROR) << __func__ << " Callback for erased [" << that << "]";
      return;
    }
  }

  std::string string_value(str_value ? str_value : std::string());
  that->task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlatformAPIWebOSGmp::DispatchCallback,
                 base::Unretained(that), type, num_value, string_value));
}

MediaPlatformAPIWebOSGmp::MediaPlatformAPIWebOSGmp(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    bool video,
    const std::string& app_id,
    const PipelineStatusCB& error_cb)
    : ls_client_(media::LunaServiceClient::PrivateBus),
      task_runner_(task_runner),
      app_id_(app_id),
      error_cb_(error_cb),
      state_(State::INVALID),
      feeded_audio_pts_(-1),
      feeded_video_pts_(-1),
      audio_eos_received_(false),
      video_eos_received_(false),
      playback_volume_(1.0),
      received_eos_(false),
      current_time_(0),
      playback_rate_(0.0f),
      play_internal_(false),
      released_media_resource_(false),
      is_destructed_(false),
      is_suspended_(false),
      load_completed_(false),
      is_finalized_(false) {
  FUNC_THIS_LOG(1);

  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id));
  media_player_client_->RegisterCallback(&MediaPlatformAPIWebOSGmp::Callback,
                                         this);
  buffer_queue_.reset(new BufferQueue());

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    g_media_apis_set_.insert(this);
  }

  SetState(State::CREATED);
}

MediaPlatformAPIWebOSGmp::~MediaPlatformAPIWebOSGmp() {
  FUNC_LOG(1);
}

void MediaPlatformAPIWebOSGmp::Initialize(
    const AudioDecoderConfig& audio_config,
    const VideoDecoderConfig& video_config,
    const PipelineStatusCB& init_cb) {
  FUNC_THIS_LOG(1);

  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!init_cb.is_null());

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  audio_config_ = audio_config;
  video_config_ = video_config;
  init_cb_ = init_cb;

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(0, &load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " Making load data info failed!";
    return;
  }

  if (is_suspended_) {
    FUNC_THIS_LOG(1) << " -> prevent background init";
    released_media_resource_ = true;
    base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
    return;
  }

  FUNC_THIS_LOG(2) << " -> call NotifyForeground";
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " media_player_client_->Load failed!";
    base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }
  ResetFeedInfo();

  base::ResetAndReturn(&init_cb_).Run(PIPELINE_OK);
}

bool MediaPlatformAPIWebOSGmp::Loaded() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return load_completed_;
}

void MediaPlatformAPIWebOSGmp::SetDisplayWindow(const gfx::Rect& rect,
                                                const gfx::Rect& in_rect,
                                                bool fullscreen) {
  FUNC_THIS_LOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  // Store display window rect info and set this if media_player_client
  // available.
  if (!media_player_client_.get()) {
    pending_set_display_window_.was_set = true;
    pending_set_display_window_.rect = rect;
    pending_set_display_window_.in_rect = in_rect;
    pending_set_display_window_.fullscreen = fullscreen;
    return;
  }

  window_rect_ = rect;
  window_in_rect_ = in_rect;

  if (window_in_rect_ == gfx::Rect() || fullscreen) {
    media_player_client_->SetDisplayWindow(window_rect_.x(), window_rect_.y(),
                                           window_rect_.width(),
                                           window_rect_.height(), fullscreen);
    LOG(INFO) << __func__ << " rect=" << rect.ToString()
              << " fullscreen=" << fullscreen;
  } else {
    media_player_client_->SetCustomDisplayWindow(
        window_in_rect_.x(), window_in_rect_.y(), window_in_rect_.width(),
        window_in_rect_.height(), window_rect_.x(), window_rect_.y(),
        window_rect_.width(), window_rect_.height(), fullscreen);
    LOG(INFO) << __func__ << " in_rect=" << in_rect.ToString()
              << " rect=" << rect.ToString() << " fullscreen=" << fullscreen;
  }
}

void MediaPlatformAPIWebOSGmp::SetLoadCompletedCb(
    const LoadCompletedCB& load_completed_cb) {
  load_completed_cb_ = load_completed_cb;
}

bool MediaPlatformAPIWebOSGmp::Feed(const scoped_refptr<DecoderBuffer>& buffer,
                                    FeedType type) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (is_finalized_ || released_media_resource_)
    return true;

  buffer_queue_->Push(buffer, type);

  while (!buffer_queue_->Empty()) {
    FeedStatus feed_status = FeedInternal(buffer_queue_->Front().first,
                                          buffer_queue_->Front().second);
    switch (feed_status) {
      case kFeedSucceeded: {
        buffer_queue_->Pop();
        continue;
      }
      case kFeedFailed: {
        LOG(ERROR) << "[" << this << "] " << __func__ << " feed failed!";
        return false;
      }
      case kFeedOverflowed: {
        if (buffer_queue_->DataSize() > kMaxPendingFeedSize) {
          LOG(INFO) << "[" << this << "] " << __func__ << " pending feed("
                    << buffer_queue_->DataSize() << ") exceeded the limit("
                    << kMaxPendingFeedSize << ")";
          return false;
        }
        LOG(INFO) << "[" << this << "] " << __func__
                  << " buffer_full: pending feed size="
                  << buffer_queue_->DataSize();
        return true;
      }
    }
  }
  return true;
}

uint64_t MediaPlatformAPIWebOSGmp::GetCurrentTime() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return current_time_;
}

bool MediaPlatformAPIWebOSGmp::Seek(base::TimeDelta time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1) << " time=" << time.InMilliseconds() << "ms";

  if (!media_player_client_)
    return true;

  ResetFeedInfo();

  if (!load_completed_) {
    // clear incompletely loaded pipeline
    if (resume_time_ != time) {
      media_player_client_.reset(NULL);
      LOG(INFO) << "[" << this << "] " << __func__
                << " Load is not finished, try to reinitialize";
      ReInitialize(time);
    }
    return true;
  }

  SetState(State::SEEKING);

  unsigned seek_time = static_cast<unsigned>(time.InMilliseconds());
  return media_player_client_->Seek(seek_time);
}

void MediaPlatformAPIWebOSGmp::SetPlaybackRate(float playback_rate) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1) << " rate(" << playback_rate_ << " -> " << playback_rate
                   << ")";

  float current_playback_rate = playback_rate_;
  playback_rate_ = playback_rate;

  if (!media_player_client_) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << " media_player_client_ is null";
    return;
  }

  if (playback_rate == 0.0f && !video_config_.IsValidConfig() &&
      audio_config_.IsValidConfig()) {
    FUNC_LOG(1) << " Play for audio only case";
    current_playback_rate = 0.0f;
    playback_rate = 1.0;
  }
  if (current_playback_rate == 0.0f && playback_rate != 0.0f) {
    FUNC_LOG(1) << " load_completed_=" << load_completed_;
    if (load_completed_)
      PlayInternal();
    else
      play_internal_ = true;
    return;
  }

  if (current_playback_rate != 0.0f && playback_rate == 0.0f) {
    FUNC_LOG(1) << " call PauseInternal()";
    PauseInternal();
  }
}

void MediaPlatformAPIWebOSGmp::Suspend(SuspendReason reason) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1) << " media_player_client_=" << media_player_client_.get()
                   << "is_finalized_=" << is_finalized_;

  if (!media_player_client_)
    return;
  is_suspended_ = true;
  if (!load_completed_)
    return;

  if (is_finalized_) {
    media_player_client_->NotifyBackground();
    return;
  }

  Unload();

  window_rect_.SetRect(0, 0, 0, 0);
  window_in_rect_.SetRect(0, 0, 0, 0);
  natural_size_.SetSize(0, 0);
}

void MediaPlatformAPIWebOSGmp::Resume(
    base::TimeDelta paused_time,
    RestorePlaybackMode restore_playback_mode) {
  FUNC_THIS_LOG(1) << " paused_time=" << paused_time.InMilliseconds() << "ms"
                   << " restore_mode="
                   << (restore_playback_mode == RESTORE_PLAYING
                           ? "RESTORE_PLAYING"
                           : "RESTORE_PAUSED");

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_suspended_ = false;
  if (released_media_resource_) {
    if (playback_rate_ == 0.0f)
      play_internal_ = false;
    task_runner_->PostTask(
        FROM_HERE,
        base::Bind(&MediaPlatformAPIWebOSGmp::ReInitialize, this, paused_time));
    return;
  }

  if (load_completed_)
    media_player_client_->NotifyForeground();
}

bool MediaPlatformAPIWebOSGmp::IsReleasedMediaResource() {
  FUNC_THIS_LOG(1);
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return released_media_resource_;
}

void MediaPlatformAPIWebOSGmp::SetVisibility(bool visible) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  // TODO: once SetVisibility api is ready, below workaround will be removed
  if (visible == false)
    SetDisplayWindow(gfx::Rect(0, 0, 1, 1), gfx::Rect(0, 0, 1, 1), false);
  has_visibility_ = visible;
}

bool MediaPlatformAPIWebOSGmp::Visibility() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return has_visibility_;
}

bool MediaPlatformAPIWebOSGmp::AllowedFeedVideo() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return video_config_.IsValidConfig() && state_ == State::PLAYING &&
         buffer_queue_->Empty() &&
         (audio_config_.IsValidConfig()
              ?  // preventing video overrun
              feeded_video_pts_ - feeded_audio_pts_ < 1000000000
              : feeded_video_pts_ - current_time_ < 3000000000);
}

bool MediaPlatformAPIWebOSGmp::AllowedFeedAudio() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return audio_config_.IsValidConfig() && state_ == State::PLAYING &&
         buffer_queue_->Empty();
}

void MediaPlatformAPIWebOSGmp::Finalize() {
  FUNC_THIS_LOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  is_destructed_ = true;
  is_finalized_ = true;

  {
    std::lock_guard<std::mutex> lock(g_media_apis_set_lock_);
    g_media_apis_set_.erase(this);
  }

  if (media_player_client_.get())
    media_player_client_.reset(NULL);

  ResetFeedInfo();
}

void MediaPlatformAPIWebOSGmp::SetPlaybackVolume(double volume) {
  FUNC_THIS_LOG(1) << " " << playback_volume_ << " -> " << volume;

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!load_completed_) {
    playback_volume_ = volume;
    return;
  }

  if (playback_volume_ == volume)
    return;

  SetVolumeInternal(volume);

  playback_volume_ = volume;
}

bool MediaPlatformAPIWebOSGmp::IsEOSReceived() {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  return received_eos_;
}

std::string MediaPlatformAPIWebOSGmp::GetMediaID(void) {
  FUNC_THIS_LOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (!media_player_client_)
    return std::string();

  const char* media_id = media_player_client_->GetMediaID();
  return media_id ? media_id : std::string();
}

void MediaPlatformAPIWebOSGmp::Unload() {
  FUNC_THIS_LOG(1);

  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);

  if (load_completed_) {
    load_completed_ = false;
    released_media_resource_ = true;

    if (media_player_client_) {
      is_finalized_ = true;
      LOG(INFO) << "[" << this << "] " << __func__
                << " destroy media client id(" << GetMediaID().c_str() << ")";
      media_player_client_.reset(NULL);
    }
  }
}

void MediaPlatformAPIWebOSGmp::UpdateVideoConfig(
    const VideoDecoderConfig& video_config) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1);
  video_config_ = video_config;
}

void MediaPlatformAPIWebOSGmp::DispatchCallback(const gint type,
                                                const gint64 num_value,
                                                const std::string& str_value) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1) << " type=" << GmpNotifyTypeToString(type)
                   << " numValue=" << num_value << " strValue=" << str_value;

  switch (static_cast<NOTIFY_TYPE_T>(type)) {
    case NOTIFY_LOAD_COMPLETED:
      NotifyLoadComplete();
      break;
    case NOTIFY_END_OF_STREAM:
      LOG(INFO) << "[" << this << "] " << __func__ << " NOTIFY_END_OF_STREAM";
      received_eos_ = true;
      break;
    case NOTIFY_CURRENT_TIME:
      if (state_ != State::SEEKING)
        UpdateCurrentTime(num_value);
      break;
    case NOTIFY_SEEK_DONE: {
      LOG(INFO) << "[" << this << "] " << __func__
                << " NOTIFY_SEEK_DONE, playback_rate_" << playback_rate_;
      if (playback_rate_ > 0.0f)
        PlayInternal();
      else
        SetState(State::PLAYING);
      break;
    }
    case NOTIFY_UNLOAD_COMPLETED:
    case NOTIFY_PLAYING:
    case NOTIFY_PAUSED:
    case NOTIFY_NEED_DATA:
    case NOTIFY_ENOUGH_DATA:
    case NOTIFY_SEEK_DATA:
      break;
    case NOTIFY_ERROR:
      LOG(INFO) << "[" << this << "] " << __func__ << " error=" << num_value;
      if (num_value == GMP_ERROR_RES_ALLOC) {
        is_finalized_ = true;
        released_media_resource_ = true;
        load_completed_ = false;
        media_player_client_.reset(NULL);
      } else if (num_value == GMP_ERROR_STREAM) {
        if (!error_cb_.is_null())
          base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
      } else if (num_value == GMP_ERROR_ASYNC) {
        if (!error_cb_.is_null())
          base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_ABORT);
      }
      break;
    case NOTIFY_VIDEO_INFO:
      UpdateVideoInfo(str_value);
      break;
    case NOTIFY_AUDIO_INFO:
    case NOTIFY_BUFFER_FULL:
    case NOTIFY_BUFFER_NEED:
      break;
    default:
      LOG(INFO) << "[" << this << "] " << __func__
                << " default case type=" << type;
      break;
  }
}

void MediaPlatformAPIWebOSGmp::PlayInternal() {
  FUNC_THIS_LOG(1);

  if (load_completed_) {
    media_player_client_->Play();
    SetState(State::PLAYING);
  }

  play_internal_ = true;
}

void MediaPlatformAPIWebOSGmp::PauseInternal(bool update_media) {
  FUNC_THIS_LOG(1) << " media_player_client=" << media_player_client_.get();

  if (!media_player_client_)
    return;

  if (update_media)
    media_player_client_->Pause();

  SetState(State::PAUSED);
}

void MediaPlatformAPIWebOSGmp::SetVolumeInternal(double volume) {
  FUNC_THIS_LOG(1) << " volume=" << volume;

  Json::Value root;
  Json::FastWriter writer;

  root["volume"] = (int)(volume * 100);
  std::string param = writer.write(root);

  // Send input volume to audiod
  ls_client_.callASync("luna://com.webos.service.audio/media/setVolume", param);
}

MediaPlatformAPIWebOSGmp::FeedStatus MediaPlatformAPIWebOSGmp::FeedInternal(
    const scoped_refptr<DecoderBuffer>& buffer,
    FeedType type) {
  if (is_finalized_ || released_media_resource_)
    return kFeedSucceeded;

  uint64_t pts = buffer->timestamp().InMicroseconds() * 1000;
  const uint8_t* data = buffer->data();
  size_t size = buffer->data_size();

  if (buffer->end_of_stream()) {
    LOG(INFO) << "[" << this << "] " << __func__ << " EOS("
              << (type == Audio ? "Audio" : "Video") << ") received!";
    if (type == Audio)
      audio_eos_received_ = true;
    else
      video_eos_received_ = true;

    if (audio_eos_received_ && video_eos_received_)
      PushEOS();

    return kFeedSucceeded;
  }

  MEDIA_DATA_CHANNEL_T es_type = MEDIA_DATA_CH_NONE;
  if (type == Video)
    es_type = MEDIA_DATA_CH_A;
  else if (type == Audio)
    es_type = MEDIA_DATA_CH_B;

  const guint8* p_buffer = static_cast<const guint8*>(data);

  MEDIA_STATUS_T media_status =
      media_player_client_->Feed(p_buffer, size, pts, es_type);
  if (media_status != MEDIA_OK) {
    LOG(INFO) << "[" << this << "] " << __func__
              << " media_status=" << media_status;
    if (media_status == MEDIA_BUFFER_FULL)
      return kFeedOverflowed;
    return kFeedFailed;
  }

  if (type == Audio)
    feeded_audio_pts_ = pts;
  else
    feeded_video_pts_ = pts;

  return kFeedSucceeded;
}

void MediaPlatformAPIWebOSGmp::PushEOS() {
  FUNC_THIS_LOG(1);

  if (is_finalized_ || released_media_resource_)
    return;

  if (media_player_client_)
    media_player_client_->PushEndOfStream();
}

void MediaPlatformAPIWebOSGmp::ResetFeedInfo() {
  feeded_audio_pts_ = -1;
  feeded_video_pts_ = -1;
  audio_eos_received_ = !audio_config_.IsValidConfig();
  video_eos_received_ = !video_config_.IsValidConfig();
  received_eos_ = false;

  buffer_queue_->Clear();
}

void MediaPlatformAPIWebOSGmp::UpdateCurrentTime(int64_t time) {
  FUNC_THIS_LOG(2) << " time=" << time;
  current_time_ = time;
}

void MediaPlatformAPIWebOSGmp::UpdateVideoInfo(const std::string& info_str) {
  FUNC_THIS_LOG(1) << " video_info=" << info_str;

  Json::Reader reader;
  Json::Value json_value;

  if (!reader.parse(info_str, json_value))
    return;

  Json::Value video_info = json_value["videoInfo"];
  uint32_t video_width = static_cast<uint32_t>(video_info["width"].asUInt());
  uint32_t video_height = static_cast<uint32_t>(video_info["height"].asUInt());

  LOG(INFO) << "[" << this << "] " << __func__ << " video_info=" << video_width
            << "x" << video_height;

  gfx::Size natural_size(video_width, video_height);
  if (natural_size_ != natural_size) {
    natural_size_ = natural_size;

    if (!size_change_cb_.is_null())
      size_change_cb_.Run();
  }
}

void MediaPlatformAPIWebOSGmp::ReInitialize(base::TimeDelta start_time) {
  std::lock_guard<std::recursive_mutex> lock(recursive_mutex_);
  FUNC_THIS_LOG(1);

  if (is_destructed_)
    return;

  if (is_finalized_)
    is_finalized_ = false;

  uint64_t pts = start_time.InMicroseconds() * 1000;
  resume_time_ = start_time;

  if (media_player_client_)
    media_player_client_.reset(NULL);

  media_player_client_.reset(new gmp::player::MediaPlayerClient(app_id_));
  media_player_client_->RegisterCallback(&MediaPlatformAPIWebOSGmp::Callback,
                                         this);

  if (pending_set_display_window_.was_set) {
    SetDisplayWindow(pending_set_display_window_.rect,
                     pending_set_display_window_.in_rect,
                     pending_set_display_window_.fullscreen);
    pending_set_display_window_.was_set = false;
    LOG(INFO) << __func__ << " pending_set_display_window_ used";
  }

  MEDIA_LOAD_DATA_T load_data;
  if (!MakeLoadData(pts, &load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << ": Making load data info failed!";
    return;
  }

  FUNC_THIS_LOG(1) << " call NotifyForeground";
  media_player_client_->NotifyForeground();

  if (!media_player_client_->Load(&load_data)) {
    LOG(ERROR) << "[" << this << "] " << __func__
               << ": media_player_client_->Load failed!";
    base::ResetAndReturn(&error_cb_).Run(PIPELINE_ERROR_DECODE);
    return;
  }

  released_media_resource_ = false;
}

void MediaPlatformAPIWebOSGmp::NotifyLoadComplete() {
  FUNC_THIS_LOG(1) << " state_=" << StateToString(state_)
                   << " play_internal_=" << play_internal_;

  load_completed_ = true;

  // Run LoadCompletedCb to TvRenderer.
  // It's necessary to callback on loading complete after feeding 10 secs.
  if (!load_completed_cb_.is_null())
    base::ResetAndReturn(&load_completed_cb_).Run();

  if (play_internal_)
    PlayInternal();

  SetVolumeInternal(playback_volume_);
}

bool MediaPlatformAPIWebOSGmp::MakeLoadData(int64_t start_time,
                                            MEDIA_LOAD_DATA_T* load_data) {
  load_data->maxWidth = 1920;
  load_data->maxHeight = 1080;
  load_data->maxFrameRate = 30;

  if (video_config_.IsValidConfig()) {
    FUNC_THIS_LOG(1) << " video_codec=" << video_config_.codec();
    Json::Value supported_codec = SupportedCodec();
    switch (video_config_.codec()) {
      case media::kCodecH264:
        load_data->maxWidth = supported_codec["H264"]["maxWidth"].asInt();
        load_data->maxHeight = supported_codec["H264"]["maxHeight"].asInt();
        load_data->maxFrameRate =
            supported_codec["H264"]["maxFrameRate"].asInt();
        break;
      case media::kCodecVP9:
        load_data->maxWidth = supported_codec["VP9"]["maxWidth"].asInt();
        load_data->maxHeight = supported_codec["VP9"]["maxHeight"].asInt();
        load_data->maxFrameRate =
            supported_codec["VP9"]["maxFrameRate"].asInt();
        break;
      case media::kCodecHEVC:
        load_data->maxWidth = supported_codec["H265"]["maxWidth"].asInt();
        load_data->maxHeight = supported_codec["H265"]["maxHeight"].asInt();
        break;
      default:
        LOG(ERROR) << "[" << this << "] " << __func__
                   << " Not Supported Video Codec(" << video_config_.codec()
                   << ")";
        return false;
    }
    load_data->videoCodec = video_codec[video_config_.codec()];
    load_data->width = video_config_.natural_size().width();
    load_data->height = video_config_.natural_size().height();
    load_data->frameRate = 30;
    load_data->extraData = (void*)video_config_.extra_data().data();
    load_data->extraSize = video_config_.extra_data().size();
  }

  if (audio_config_.IsValidConfig()) {
    FUNC_THIS_LOG(1) << " audio_codec=" << audio_config_.codec();
    load_data->audioCodec = audio_codec[audio_config_.codec()];
    load_data->channels = audio_config_.channel_layout();
    load_data->sampleRate = audio_config_.samples_per_second();
    load_data->bitRate = audio_config_.bits_per_channel();
    load_data->bitsPerSample = 8 * audio_config_.bytes_per_frame();
  }
  load_data->ptsToDecode = start_time;

  FUNC_THIS_LOG(1) << " Outgoing codec info audio=" << load_data->audioCodec
                   << " video=" << load_data->videoCodec;

  return true;
}

void MediaPlatformAPIWebOSGmp::SetKeySystem(const std::string& key_system) {}

// private helper functions
void MediaPlatformAPIWebOSGmp::SetState(State next_state) {
  LOG(INFO) << "[" << this << "] " << __func__ << " " << StateToString(state_)
            << " -> " << StateToString(next_state);

  state_ = next_state;
}

// static
const char* MediaPlatformAPIWebOSGmp::StateToString(State s) {
  static const char* state_string[] = {
      "INVALID",   "CREATED",   "CREATED_SUSPENDED", "LOADING",  "LOADED",
      "PLAYING",   "PAUSED",    "SUSPENDED",         "RESUMING", "SEEKING",
      "RESTORING", "FINALIZED",
  };
  if (s > State::FINALIZED)
    return "INVALID";
  return state_string[static_cast<int>(s)];
}

}  // namespace media
