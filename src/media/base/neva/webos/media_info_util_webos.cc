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

#include "media/base/neva/webos/media_info_util_webos.h"

#include "base/logging.h"

#define FUNC_LOG(x) DVLOG(x) << __func__

namespace media {

std::string PlaybackNotificationToJson(
    const std::string& media_id,
    const PlaybackNotification notification) {
  Json::Value eventInfo;
  Json::FastWriter writer;

  eventInfo["type"] = "playbackNotificationInfo";
  eventInfo["mediaId"] = media_id.c_str();

  switch (notification) {
    case PlaybackNotification::NotifySeekDone:
      eventInfo["info"] = "seekDone";
      break;
    case PlaybackNotification::NotifyPlaying:
      eventInfo["info"] = "playing";
      break;
    case PlaybackNotification::NotifyPaused:
      eventInfo["info"] = "paused";
      break;
    case PlaybackNotification::NotifyPreloadCompleted:
      eventInfo["info"] = "preloadCompleted";
      break;
    case PlaybackNotification::NotifyLoadCompleted:
      eventInfo["info"] = "loadCompleted";
      break;
    case PlaybackNotification::NotifyEndOfStreamForward:
      eventInfo["info"] = "endOfStream";
      eventInfo["direction"] = "forward";
      break;
    case PlaybackNotification::NotifyEndOfStreamBackward:
      eventInfo["info"] = "endOfStream";
      eventInfo["direction"] = "backward";
      break;
    default:
      return std::string("");
  }

  return writer.write(eventInfo);
}

std::string ExternalSubtitleTrackInfoToJson(
    const std::string& media_id,
    const struct uMediaServer::external_subtitle_track_info_t& value) {
  Json::Value eventInfo;
  Json::Value externalSubtitleTrackInfo;
  Json::FastWriter writer;

  eventInfo["type"] = "externalSubtitleTrackInfo";
  eventInfo["mediaId"] = media_id.c_str();

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

std::string ErrorInfoToJson(const std::string& media_id,
                            int64_t errorCode,
                            const std::string& errorText) {
  Json::Value eventInfo;
  Json::Value error;
  Json::FastWriter writer;

  eventInfo["type"] = "error";
  eventInfo["mediaId"] = media_id.c_str();

  error["errorCode"] = errorCode;
  error["errorText"] = errorText;

  eventInfo["info"] = error;

  return writer.write(eventInfo);
}

std::string UserDefinedInfoToJson(const std::string& media_id,
                                  const std::string& message) {
  Json::Value eventInfo;
  Json::Value userDefinedChanged;
  Json::Reader reader;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "userDefinedChanged";
  eventInfo["mediaId"] = media_id.c_str();

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

  return res;
}

std::string MasterClockInfoToJson(
    const std::string& media_id,
    const struct uMediaServer::master_clock_info_t& masterClockInfo) {
  Json::Value eventInfo;
  Json::Value setMasterClockResult;
  Json::FastWriter writer;

  eventInfo["type"] = "setMasterClockResult";
  eventInfo["mediaId"] = media_id.c_str();

  setMasterClockResult["result"] = masterClockInfo.result;
  setMasterClockResult["port"] = masterClockInfo.port;
  setMasterClockResult["baseTime"] = masterClockInfo.baseTime;

  eventInfo["info"] = setMasterClockResult;
  return writer.write(eventInfo);
}

std::string SlaveClockInfoToJson(
    const std::string& media_id,
    const struct uMediaServer::slave_clock_info_t& slaveClockInfo) {
  Json::Value eventInfo;
  Json::Value setSlaveClockResult;
  Json::FastWriter writer;

  eventInfo["type"] = "setSlaveClockResult";
  eventInfo["mediaId"] = media_id.c_str();

  setSlaveClockResult["result"] = slaveClockInfo.result;

  eventInfo["info"] = setSlaveClockResult;
  return writer.write(eventInfo);
}

}  // namespace media
