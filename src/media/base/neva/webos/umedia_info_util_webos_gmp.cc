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

#include "media/base/neva/webos/umedia_info_util_webos_gmp.h"

#include "base/logging.h"

#define FUNC_LOG(x) DVLOG(x) << __func__

namespace media {

std::string SourceInfoToJson(const std::string& media_id,
                             const struct ums::source_info_t& value) {
  Json::Value eventInfo;
  Json::Value sourceInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "sourceInfo";
  eventInfo["mediaId"] = media_id.c_str();

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
  return res;
}

// refer to uMediaServer/include/public/dto_type.h
std::string VideoInfoToJson(const std::string& media_id,
                            const struct ums::video_info_t& value) {
  Json::Value eventInfo;
  Json::Value videoInfo;
  Json::Value frameRate;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "videoInfo";
  eventInfo["mediaId"] = media_id.c_str();

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
  return res;
}

std::string AudioInfoToJson(const std::string& media_id,
                            const struct ums::audio_info_t& value) {
  Json::Value eventInfo;
  Json::Value audioInfo;
  Json::FastWriter writer;
  std::string res;

  eventInfo["type"] = "audioInfo";
  eventInfo["mediaId"] = media_id.c_str();

  audioInfo["sampleRate"] = value.sample_rate;
  audioInfo["codec"] = value.codec.c_str();
  audioInfo["bitRate"] = value.bit_rate;

  eventInfo["info"] = audioInfo;
  res = writer.write(eventInfo);

  return res;
}

}  // namespace media
