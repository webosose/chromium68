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

MediaPlayerCamera::MediaPlayerCamera(
    MediaPlayerNevaClient* client,
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
    const std::string& app_id)
    : client_(client),
      main_task_runner_(base::MessageLoop::current()->task_runner()),
      has_audio_(false),
      has_video_(false),
      loaded_(false),
      current_time_(0.0f),
      display_window_(0, 0, 1920, 1080),
      fullscreen_(false) {
  FUNC_LOG(2);
  weak_ptr_ = AsWeakPtr();
}

MediaPlayerCamera::~MediaPlayerCamera() {
  FUNC_LOG(2) << " media_id : " << media_id_.c_str();
  weak_ptr_.reset();
  if (!media_id_.empty() && loaded_)
    uMediaServer::uMediaClient::unload();
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
  FUNC_LOG(2) << " app_id: " << app_id.c_str() << " url : " << url.c_str()
              << " payload - " << (payload.empty() ? "{}" : payload.c_str());

  app_id_ = app_id;
  url::Parsed parsed;
  url::ParseStandardURL(url.c_str(), url.length(), &parsed);
  url_ = GURL(url, parsed, true);
  mime_type_ = mime_type;

  Json::Value root;
  root["streamType"] = "MPEGTS";
  root["format"] = "jpeg";
  root["width"] = 1920;
  root["height"] = 1080;
  root["frameRate"] = 30;
  root["pipelineMode"] = "webkit";
  root["recordMode"] = "off";
  root["memType"] = "device";
  root["memSrc"] = "/dev/video0";
  root["option"]["appId"] = app_id_;

  if (!payload.empty()) {
    Json::Value response;
    Json::Reader reader;

    reader.parse(payload, response);
    if (response.isMember("streamType"))
      root["streamType"] = response["streamType"].asString();
    if (response.isMember("format"))
      root["format"] = response["format"].asString();
    if (response.isMember("width"))
      root["width"] = response["width"].asUInt();
    if (response.isMember("height"))
      root["height"] = response["height"].asUInt();
    if (response.isMember("frameRate"))
      root["frameRate"] = response["frameRate"].asUInt();
    if (response.isMember("pipelineMode"))
      root["pipelineMode"] = response["pipelineMode"].asString();
    if (response.isMember("recordMode"))
      root["recordMode"] = response["recordMode"].asString();
    if (response.isMember("memType"))
      root["memType"] = response["memType"].asString();
    if (response.isMember("memSrc"))
      root["memSrc"] = response["memSrc"].asString();
  }

  uMediaServer::uMediaClient::notifyForeground();

  Json::FastWriter writer;
  FUNC_LOG(2) << " Call uMediaClient::loadAsync( " << url_.spec() << " ) ("
              << writer.write(root) << " )";
  uMediaServer::uMediaClient::loadAsync(url_.spec().c_str(), kCamera,
                                        writer.write(root).c_str());
}

void MediaPlayerCamera::Start() {
  FUNC_LOG(2);

  if (!time_update_timer_.IsRunning()) {
    time_update_timer_.Start(FROM_HERE, base::TimeDelta::FromMilliseconds(
                                            media::kTimeUpdateInterval),
                             this, &MediaPlayerCamera::UpdateCurrentTimeFired);
  }
}

void MediaPlayerCamera::Pause() {
  FUNC_LOG(2);

  time_update_timer_.Stop();

  uMediaServer::uMediaClient::pause();
}

void MediaPlayerCamera::SetRate(double playback_rate) {
  FUNC_LOG(2) << " playback_rate =" << playback_rate;
}

bool MediaPlayerCamera::IsPreloadable(const std::string& content_media_option) {
  FUNC_LOG(2);
  return false;
}

void MediaPlayerCamera::SetDisplayWindow(const gfx::Rect& out_rect,
                                         const gfx::Rect& in_rect,
                                         bool full_screen,
                                         bool forced) {
  FUNC_LOG(2) << " full_screen: " << full_screen << " forced:" << forced;

  using namespace uMediaServer;

  display_window_out_rect_ = out_rect;
  display_window_in_rect_ = in_rect;

  if (out_rect.IsEmpty())
    return;

  if (!forced && display_window_ == out_rect)
    return;

  display_window_ = out_rect;

  if (full_screen) {
    switchToFullscreen();
  } else if (in_rect.IsEmpty()) {
    LOG(INFO) << __PRETTY_FUNCTION__ << " out_rect=[ " << out_rect.ToString()
              << " ]";

    uMediaClient::setDisplayWindow(rect_t(out_rect.x(), out_rect.y(),
                                          out_rect.width(), out_rect.height()));
  } else {
    LOG(INFO) << __PRETTY_FUNCTION__ << " inRect=[" << in_rect.ToString() << "]"
              << " outRect=[" << out_rect.ToString() << "]";

    uMediaClient::setDisplayWindow(
        rect_t(in_rect.x(), in_rect.y(), in_rect.width(), in_rect.height()),
        rect_t(out_rect.x(), out_rect.y(), out_rect.width(),
               out_rect.height()));
  }
}

bool MediaPlayerCamera::onLoadCompleted() {
  FUNC_LOG(2) << " loaded_=" << loaded_;

  if (loaded_)
    return true;

  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchLoadCompleted, weak_ptr_));
  return true;
}

bool MediaPlayerCamera::onUnloadCompleted() {
  FUNC_LOG(2) << " loaded_=" << loaded_;
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchUnloadCompleted, weak_ptr_));
  return true;
}

bool MediaPlayerCamera::onPlaying() {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchPlaying, weak_ptr_));
  return true;
}

bool MediaPlayerCamera::onPaused() {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchPaused, weak_ptr_));
  return true;
}

#if UMS_INTERNAL_API_VERSION == 2
bool MediaPlayerCamera::onAudioInfo(const ums::audio_info_t& audioInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchAudioInfo, weak_ptr_, audioInfo));
  return true;
}

bool MediaPlayerCamera::onVideoInfo(const ums::video_info_t& videoInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchVideoInfo, weak_ptr_, videoInfo));
  return true;
}

bool MediaPlayerCamera::onSourceInfo(const ums::source_info_t& sourceInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchSourceInfo, weak_ptr_,
                            sourceInfo));
  return true;
}
#else
bool MediaPlayerCamera::onAudioInfo(
    const uMediaServer::audio_info_t& audioInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchAudioInfo, weak_ptr_, audioInfo));
  return true;
}

bool MediaPlayerCamera::onVideoInfo(
    const uMediaServer::video_info_t& videoInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchVideoInfo, weak_ptr_, videoInfo));
  return true;
}

bool MediaPlayerCamera::onSourceInfo(
    const uMediaServer::source_info_t& sourceInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchSourceInfo, weak_ptr_,
                            sourceInfo));
  return true;
}
#endif

bool MediaPlayerCamera::onCurrentTime(int64_t currentTime) {
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchCurrentTime, weak_ptr_,
                            currentTime));
  return true;
}

bool MediaPlayerCamera::onError(long long errorCode,
                                const std::string& errorText) {
  FUNC_LOG(2) << " onError : " << errorCode << ", " << errorText;
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchError, weak_ptr_,
                            errorCode, errorText));
  return true;
}

bool MediaPlayerCamera::onEndOfStream() {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchEndOfStream, weak_ptr_));
  return true;
}

bool MediaPlayerCamera::onFileGenerated() {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE,
      base::Bind(&MediaPlayerCamera::DispatchFileGenerated, weak_ptr_));
  return true;
}

bool MediaPlayerCamera::onRecordInfo(
    const uMediaServer::record_info_t& recordInfo) {
  FUNC_LOG(2);
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchRecordInfo, weak_ptr_,
                            recordInfo));
  return true;
}

bool MediaPlayerCamera::onUserDefinedChanged(const char* payload) {
  std::string message = payload ? std::string(payload) : "";
  FUNC_LOG(2) << " message : " << message;
  main_task_runner_->PostTask(
      FROM_HERE, base::Bind(&MediaPlayerCamera::DispatchUserDefinedChanged,
                            weak_ptr_, message));
  return true;
}

void MediaPlayerCamera::DispatchLoadCompleted() {
  media_id_ = std::string(uMediaServer::uMediaClient::media_id);
  FUNC_LOG(2) << " loadCompleted : " << media_id_;

  if (loaded_) {
    LOG(INFO) << __func__ << "ignore duplicated loadCompleted event";
    return;
  }

  loaded_ = true;

  if (client_) {
    SetDisplayWindow(display_window_out_rect_, display_window_in_rect_,
                     fullscreen_, true);

    FUNC_LOG(2) << " width: " << display_window_.width()
                << " height: " << display_window_.height();
    client_->OnMediaMetadataChanged(kInfiniteDuration, display_window_.width(),
                                    display_window_.height(), true);

    Json::Value root;
    root["mediaId"] = media_id_.c_str();
    root["cameraState"] = "loaded";

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }

  uMediaServer::uMediaClient::play();
}

void MediaPlayerCamera::DispatchUnloadCompleted() {
  FUNC_LOG(2) << " UnloadCompleted : " << media_id_;
  loaded_ = false;
}

void MediaPlayerCamera::DispatchPlaying() {
  FUNC_LOG(2);
  if (client_) {
    client_->OnMediaPlayerPlay();

    Json::Value root;
    root["mediaId"] = media_id_.c_str();
    root["infoType"] = "cameraState";
    root["cameraState"] = "playing";

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchPaused() {
  FUNC_LOG(2);
  if (client_) {
    client_->OnMediaPlayerPause();

    Json::Value root;
    root["mediaId"] = media_id_.c_str();
    root["infoType"] = "cameraState";
    root["cameraState"] = "paused";

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchUserDefinedChanged(const std::string& message) {
  FUNC_LOG(2) << " message : " << message;

  Json::Value root;
  Json::Reader reader;
  Json::FastWriter writer;
  std::string param;

  reader.parse(message, root);

  root["infoType"] = "acquire";
  root["mediaId"] = media_id_.c_str();
  if (root.isMember("cameraId")) {
    camera_id_ = root["cameraId"].asString();
    FUNC_LOG(2) << " cameraId received : " << camera_id_;
    param = writer.write(root);
  } else if (root.isMember("cameraServiceEvent")) {
    param = writer.write(root["cameraServiceEvent"]);
  } else {
    param = writer.write(root);
  }

  if (client_) {
    FUNC_LOG(2) << " param : " << param;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState, param);
  }
}

#if UMS_INTERNAL_API_VERSION == 2
void MediaPlayerCamera::DispatchAudioInfo(
    const struct ums::audio_info_t& audioInfo) {
  FUNC_LOG(2);

  has_audio_ = true;

  if (client_) {
    Json::Value audio;
    audio["sampleRate"] = audioInfo.sample_rate;
    audio["codec"] = audioInfo.codec.c_str();
    audio["bitRate"] = audioInfo.bit_rate;

    Json::Value root;
    root["infoType"] = "audioInfo";
    root["mediaId"] = media_id_.c_str();
    root["info"] = audio;

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchVideoInfo(
    const struct ums::video_info_t& videoInfo) {
  FUNC_LOG(2);

  has_video_ = true;

  gfx::Size natural_video_size(videoInfo.width, videoInfo.height);
  if (natural_video_size_ != natural_video_size) {
    natural_video_size_ = natural_video_size;

    display_window_.set_width(videoInfo.width);
    display_window_.set_height(videoInfo.height);

    if (client_)
      client_->OnVideoSizeChanged(display_window_.width(),
                                  display_window_.height());
  }

  if (client_) {
    Json::Value video;
    video["width"] = videoInfo.width;
    video["height"] = videoInfo.height;
    video["codec"] = videoInfo.codec.c_str();
    video["bitRate"] = videoInfo.bit_rate;

    Json::Value frameRate;
    frameRate["num"] = videoInfo.frame_rate.num;
    frameRate["den"] = videoInfo.frame_rate.den;
    video["frameRate"] = frameRate;

    Json::Value root;
    root["infoType"] = "videoInfo";
    root["mediaId"] = media_id_.c_str();
    root["info"] = video;

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchSourceInfo(
    const struct ums::source_info_t& sourceInfo) {
  FUNC_LOG(2);

  if (sourceInfo.programs.size() > 0) {
    uint32_t video_stream = sourceInfo.programs[0].video_stream;
    if (video_stream > 0 && video_stream < sourceInfo.video_streams.size()) {
      ums::video_info_t video_info = sourceInfo.video_streams[video_stream];
      gfx::Size natural_video_size(video_info.width, video_info.height);
      if (natural_video_size_ != natural_video_size) {
        natural_video_size_ = natural_video_size;
        if (client_)
          client_->OnVideoSizeChanged(video_info.width, video_info.height);

        display_window_.set_width(video_info.width);
        display_window_.set_height(video_info.height);
      }
    }
  }

  if (client_) {
    Json::Value source;
    source["container"] = sourceInfo.container.c_str();
    source["seekable"] = sourceInfo.seekable;
    source["numPrograms"] = sourceInfo.programs.size();

    Json::Value root;
    root["infoType"] = "sourceInfo";
    root["mediaId"] = media_id_.c_str();
    root["info"] = source;

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));

    client_->OnMediaMetadataChanged(kInfiniteDuration, display_window_.width(),
                                    display_window_.height(), true);
  }
}
#else
void MediaPlayerCamera::DispatchAudioInfo(
    const struct uMediaServer::audio_info_t& audioInfo) {
  FUNC_LOG(2);
  has_audio_ = true;
  if (client_) {
    Json::Value root;
    root["infoType"] = "audioInfo";
    root["sampleRate"] = audioInfo.sampleRate;
    root["channels"] = audioInfo.channels;
    root["mediaId"] = media_id_.c_str();

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchVideoInfo(
    const struct uMediaServer::video_info_t& videoInfo) {
  FUNC_LOG(2);
  has_video_ = true;
  if (client_) {
    Json::Value root;
    root["infoType"] = "videoInfo";
    root["width"] = videoInfo.width;
    root["height"] = videoInfo.height;
    root["aspectRatio"] = videoInfo.aspectRatio;
    root["frameRate"] = videoInfo.frameRate;
    root["mode3D"] = videoInfo.mode3D;
    root["mediaId"] = media_id_.c_str();

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchSourceInfo(
    const struct uMediaServer::source_info_t& sourceInfo) {
  FUNC_LOG(2);
}
#endif

void MediaPlayerCamera::DispatchCurrentTime(int64_t currentTime) {
  current_time_ = currentTime / 1000.;
}

void MediaPlayerCamera::DispatchError(long long errorCode,
                                      const std::string& errorText) {
  FUNC_LOG(2);
  Json::Value root;
  Json::Reader reader;

  root["mediaId"] = media_id_.c_str();

  reader.parse(errorText, root);
  if (errorCode == ImageDecodeError || errorCode == ImageDisplayError) {
    if (root.isMember("id"))
      camera_id_ = root["id"].asString();
    root["infoType"] = "acquire";
  } else {
    root["infoType"] = "error";
    root["errorCode"] = static_cast<double>(errorCode);
    root["errorText"] = errorText;
  }

  Json::FastWriter writer;
  if (client_)
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
}

void MediaPlayerCamera::DispatchEndOfStream() {
  FUNC_LOG(2);
  if (client_) {
    Json::Value root;
    root["infoType"] = "endOfStream";
    root["mediaId"] = media_id_.c_str();

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchFileGenerated() {
  FUNC_LOG(2);
  if (client_) {
    Json::Value root;
    root["infoType"] = "fileGenerated";
    root["mediaId"] = media_id_.c_str();

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::DispatchRecordInfo(
    const uMediaServer::record_info_t& recordInfo) {
  FUNC_LOG(2);
  if (client_) {
    Json::Value root;
    root["infoType"] = "recordInfo";
    root["recordState"] = recordInfo.recordState;
    root["elapsedMiliSecond"] = recordInfo.elapsedMiliSecond;
    root["bitRate"] = recordInfo.bitRate;
    root["fileSize"] = recordInfo.fileSize;
    root["fps"] = recordInfo.fps;
    root["mediaId"] = media_id_.c_str();

    Json::FastWriter writer;
    client_->OnCustomMessage(
        blink::WebMediaPlayer::kMediaEventUpdateCameraState,
        writer.write(root));
  }
}

void MediaPlayerCamera::UpdateCurrentTimeFired() {
  FUNC_LOG(2);
  base::TimeDelta current_time = base::TimeDelta::FromSecondsD(current_time_);
  if (last_time_update_time_ == current_time)
    return;
  if (client_)
    client_->OnTimeUpdate(current_time, base::TimeTicks::Now());
  last_time_update_time_ = current_time;
}

}  // namespace media
