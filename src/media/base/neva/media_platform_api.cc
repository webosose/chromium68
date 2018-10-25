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

#include "media/base/neva/media_platform_api.h"

namespace media {

MediaPlatformAPI::BufferQueue::BufferQueue() : data_size_(0) {}
void MediaPlatformAPI::BufferQueue::Push(
    const scoped_refptr<DecoderBuffer>& buffer,
    MediaPlatformAPI::FeedType type) {
  queue_.push(DecoderBufferPair(buffer, type));
  data_size_ += buffer->data_size();
}

const std::pair<scoped_refptr<DecoderBuffer>, MediaPlatformAPI::FeedType>
MediaPlatformAPI::BufferQueue::Front() {
  return queue_.front();
}

void MediaPlatformAPI::BufferQueue::Pop() {
  std::pair<scoped_refptr<DecoderBuffer>, MediaPlatformAPI::FeedType> f =
      queue_.front();
  data_size_ -= f.first->data_size();
  queue_.pop();
}

bool MediaPlatformAPI::BufferQueue::Empty() {
  return queue_.empty();
}

void MediaPlatformAPI::BufferQueue::Clear() {
  DecoderBufferQueue empty_queue;
  queue_.swap(empty_queue);
}

size_t MediaPlatformAPI::BufferQueue::DataSize() const {
  return data_size_;
}

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
