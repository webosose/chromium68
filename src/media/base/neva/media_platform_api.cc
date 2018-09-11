// Copyright (c) 2018 LG Electronics, Inc.

#include "media/base/neva/media_platform_api.h"

namespace media {

MediaPlatformAPI::MediaPlatformAPI() {
}

MediaPlatformAPI::~MediaPlatformAPI() {
}

Json::Value MediaPlatformAPI::supported_codec_ = Json::Value();

base::Optional<MediaTypeRestriction> MediaPlatformAPI::GetPlatformRestrictionForType(
    const std::string& type) {
  if (supported_codec_[type].empty())
    return base::nullopt;

  MediaTypeRestriction restriction =
      MediaTypeRestriction(supported_codec_[type]["maxWidth"].asInt(),
                           supported_codec_[type]["maxHeight"].asInt(),
                           supported_codec_[type]["maxFrameRate"].asInt(),
                           supported_codec_[type]["maxBitRate"].asInt(),
                           supported_codec_[type]["channels"].asInt());

  return restriction;
}

void MediaPlatformAPI::SetMediaCodecCapability(const std::string& codec_info) {
  Json::Value codec_capability;
  Json::Reader reader;

  if (!supported_codec_.isNull())
    return;

  if (!reader.parse(codec_info, codec_capability))
    return;

  Json::Value videoCodecs = codec_capability["videoCodecs"];
  Json::Value audioCodecs = codec_capability["audioCodecs"];

  for (Json::Value::iterator iter = videoCodecs.begin();
       iter != videoCodecs.end(); iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "H.264")
        supported_codec_["video/mp4"] = *iter;
      else if ((*iter)["name"].asString() == "VP9")
        supported_codec_["video/vp9"] = *iter;
    }
  }

  for (Json::Value::iterator iter = audioCodecs.begin();
       iter != audioCodecs.end(); iter++) {
    if ((*iter).isObject()) {
      if ((*iter)["name"].asString() == "AAC")
        supported_codec_["audio/mp4"] = *iter;
    }
  }
}

}  // namespace media
