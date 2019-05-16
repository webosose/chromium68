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

#include "media/blink/neva/webos/mediaplayer_camera.h"

#include <algorithm>

#include "base/bind.h"
#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/logging.h"
#include "media/base/bind_to_current_loop.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define FUNC_LOG(x) VLOG(x) << __func__

namespace media {

#define BIND_TO_RENDER_LOOP(function)                   \
  (DCHECK(main_task_runner_->BelongsToCurrentThread()), \
   media::BindToCurrentLoop(base::Bind(function, AsWeakPtr())))

MediaPlayerCamera::MediaPlayerCamera(
    MediaPlayerNevaClient* client,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner,
    const std::string& app_id)
    : client_(client),
      playback_rate_(1.0f),
      is_video_offscreen_(false),
      fullscreen_(false),
      main_task_runner_(main_task_runner) {
  LOG(INFO) << __func__;
  umedia_client_ = WebOSMediaClient::Create(main_task_runner_, app_id);
}

MediaPlayerCamera::~MediaPlayerCamera() {
  LOG(INFO) << __func__;
  DCHECK(main_task_runner_->BelongsToCurrentThread());
}

void MediaPlayerCamera::Initialize(const bool is_video,
                                   const double current_time,
                                   const std::string& app_id,
                                   const std::string& url,
                                   const std::string& mime_type,
                                   const std::string& referrer,
                                   const std::string& user_agent,
                                   const std::string& cookies,
                                   const std::string& payload) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2) << " app_id: " << app_id.c_str() << " url : " << url.c_str()
              << " payload - " << (payload.empty() ? "{}" : payload.c_str());

  app_id_ = app_id;
  url::Parsed parsed;
  url::ParseStandardURL(url.c_str(), url.length(), &parsed);
  url_ = GURL(url, parsed, true);
  mime_type_ = mime_type;

  umedia_client_->Load(
      is_video, current_time, false, app_id, url, mime_type, referrer,
      user_agent, cookies, payload,
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnPlaybackStateChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnStreamEnded),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnSeekDone),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnError),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnBufferingState),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnDurationChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnVideoSizeChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnVideoDisplayWindowChange),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnAddAudioTrack),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnAddVideoTrack),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnUpdateUMSInfo),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnAudioFocusChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnActiveRegionChanged),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnWaitingForDecryptionKey),
      BIND_TO_RENDER_LOOP(&MediaPlayerCamera::OnEncryptedMediaInitData));
}

void MediaPlayerCamera::Start() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);

  umedia_client_->SetPlaybackRate(playback_rate_);
  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(
                                            media::kTimeUpdateInterval),
                             this, &MediaPlayerCamera::OnTimeUpdateTimerFired);
  }
}

void MediaPlayerCamera::Pause() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);

  time_update_timer_.Stop();
}

bool MediaPlayerCamera::IsPreloadable(const std::string& content_media_option) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return false;
}

bool MediaPlayerCamera::HasVideo() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);
  return umedia_client_->HasVideo();
}

bool MediaPlayerCamera::HasAudio() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);
  return umedia_client_->HasAudio();
}

bool MediaPlayerCamera::SelectTrack(const MediaTrackType type,
                                    const std::string& id) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return true;
}

void MediaPlayerCamera::SwitchToAutoLayout() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  umedia_client_->SwitchToAutoLayout();
}

void MediaPlayerCamera::SetDisplayWindow(const gfx::Rect& out_rect,
                                         const gfx::Rect& in_rect,
                                         bool full_screen,
                                         bool forced) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1) << " out_rect:" << out_rect.ToString()
              << " out_rect:" << out_rect.ToString();

  display_window_out_rect_ = out_rect;
  display_window_in_rect_ = in_rect;
  umedia_client_->SetDisplayWindow(out_rect, in_rect, full_screen, forced);
}

bool MediaPlayerCamera::UsesIntrinsicSize() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return umedia_client_->UsesIntrinsicSize();
}

std::string MediaPlayerCamera::MediaId() const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(1);
  return umedia_client_->MediaId();
}

bool MediaPlayerCamera::HasVisibility(void) const {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return umedia_client_->Visibility();
}

void MediaPlayerCamera::SetVisibility(bool visibility) {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  if (!is_video_offscreen_)
    umedia_client_->SetVisibility(visibility);
}

bool MediaPlayerCamera::RequireMediaResource() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  return true;
}

void MediaPlayerCamera::OnPlaybackStateChanged(bool playing) {
  FUNC_LOG(1);

  if (!client_)
    return;

  Json::Value root;
  root["mediaId"] = MediaId().c_str();
  root["infoType"] = "cameraState";

  if (playing) {
    client_->OnMediaPlayerPlay();
    root["cameraState"] = "playing";
  } else {
    client_->OnMediaPlayerPause();
    root["cameraState"] = "paused";
  }

  Json::FastWriter writer;
  client_->OnCustomMessage(blink::WebMediaPlayer::kMediaEventUpdateCameraState,
                           writer.write(root));
}

void MediaPlayerCamera::OnStreamEnded() {
  FUNC_LOG(1);
  time_update_timer_.Stop();
}

void MediaPlayerCamera::OnBufferingState(
    WebOSMediaClient::BufferingState buffering_state) {
  FUNC_LOG(2) << " state:" << buffering_state;

  switch (buffering_state) {
    case WebOSMediaClient::kHaveMetadata: {
      umedia_client_->SetPlaybackRate(playback_rate_);
      gfx::Size videoSize = umedia_client_->GetNaturalVideoSize();
      client_->OnMediaMetadataChanged(
          base::TimeDelta::FromSecondsD(umedia_client_->GetDuration()),
          videoSize.width(), videoSize.height(), true);
      break;
    }
    case WebOSMediaClient::kLoadCompleted:
    case WebOSMediaClient::kPreloadCompleted:
      client_->OnLoadComplete();
      break;
    case WebOSMediaClient::kPrerollCompleted:
    case WebOSMediaClient::kWebOSBufferingStart:
    case WebOSMediaClient::kWebOSBufferingEnd:
    case WebOSMediaClient::kWebOSNetworkStateLoading:
    case WebOSMediaClient::kWebOSNetworkStateLoaded:
    default:
      break;
  }
}

void MediaPlayerCamera::OnVideoSizeChange() {
  FUNC_LOG(1);
  gfx::Size size = umedia_client_->GetNaturalVideoSize();
  client_->OnVideoSizeChanged(size.width(), size.height());
}

void MediaPlayerCamera::OnVideoDisplayWindowChange() {
  FUNC_LOG(1);
  umedia_client_->SetDisplayWindow(display_window_out_rect_,
                                   display_window_in_rect_, fullscreen_, true);
}

void MediaPlayerCamera::OnAddAudioTrack(
    const std::vector<struct MediaTrackInfo>& audio_track_info) {
  NOTIMPLEMENTED();
}

void MediaPlayerCamera::OnAddVideoTrack(const std::string& id,
                                        const std::string& kind,
                                        const std::string& language,
                                        bool enabled) {
  NOTIMPLEMENTED();
}

void MediaPlayerCamera::OnUpdateUMSInfo(const std::string& detail) {
  FUNC_LOG(1);

  if (!client_ || detail.empty())
    return;

  Json::Reader reader;
  Json::Value root;
  if (!reader.parse(detail, root)) {
    LOG(ERROR) << " Json::Reader::parse (" << detail << ") - Failed!!!";
    return;
  }

  if (root.isMember("cameraId")) {
    camera_id_ = root["cameraId"].asString();
    FUNC_LOG(2) << " cameraId received : " << camera_id_;
  }

  client_->OnCustomMessage(blink::WebMediaPlayer::kMediaEventUpdateCameraState,
                           detail);
}

void MediaPlayerCamera::OnActiveRegionChanged(const gfx::Rect& active_region) {
  FUNC_LOG(1) << gfx::Rect(active_region).ToString();

  if (active_video_region_ != active_region)
    active_video_region_ = active_region;

  client_->OnActiveRegionChanged(blink::WebRect(
      active_video_region_.x(), active_video_region_.y(),
      active_video_region_.width(), active_video_region_.height()));
}

void MediaPlayerCamera::OnEncryptedMediaInitData(
    const std::string& init_data_type,
    const std::vector<uint8_t>& init_data) {
  NOTIMPLEMENTED();
}

base::TimeDelta MediaPlayerCamera::GetCurrentTime() {
  DCHECK(main_task_runner_->BelongsToCurrentThread());
  FUNC_LOG(2);
  return base::TimeDelta::FromSecondsD(umedia_client_->GetCurrentTime());
}

void MediaPlayerCamera::OnTimeUpdateTimerFired() {
  FUNC_LOG(2);
  if (client_)
    client_->OnTimeUpdate(GetCurrentTime(), base::TimeTicks::Now());
}

}  // namespace media
