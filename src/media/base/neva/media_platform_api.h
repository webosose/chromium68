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

#ifndef MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H
#define MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H

#include <queue>
#include <set>
#include "base/optional.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/decoder_buffer.h"
#include "media/base/neva/media_type_restriction.h"
#include "media/base/pipeline_status.h"
#include "media/base/video_decoder_config.h"
#include "ui/gfx/geometry/rect.h"
#include "third_party/jsoncpp/source/include/json/json.h"

namespace media {

class MEDIA_EXPORT MediaPlatformAPI
    : public base::RefCountedThreadSafe<media::MediaPlatformAPI> {
 public:
  enum FeedType {
    Video = 1,
    Audio,
  };

  enum RestorePlaybackMode {
    RESTORE_PAUSED,
    RESTORE_PLAYING
  };

  MediaPlatformAPI();

  virtual void Initialize(const AudioDecoderConfig& audio_config,
                          const VideoDecoderConfig& video_config,
                          const PipelineStatusCB& init_cb) = 0;
  virtual void SetDisplayWindow(const gfx::Rect& rect,
                                const gfx::Rect& in_rect,
                                bool fullscreen) = 0;
  virtual bool Feed(const scoped_refptr<DecoderBuffer>& buffer,
                    FeedType type) = 0;
  virtual uint64_t GetCurrentTime() = 0;
  virtual bool Seek(base::TimeDelta time) = 0;
  virtual void Suspend() = 0;
  virtual void Resume(base::TimeDelta paused_time,
                      RestorePlaybackMode restore_playback_mode) = 0;
  virtual void SetPlaybackRate(float playback_rate) = 0;
  virtual void SetPlaybackVolume(double volume) = 0;
  virtual bool AllowedFeedVideo() = 0;
  virtual bool AllowedFeedAudio() = 0;
  virtual void Finalize() = 0;
  virtual void SetKeySystem(const std::string& key_system) = 0;
  virtual bool IsEOSReceived() = 0;
  virtual void UpdateVideoConfig(const VideoDecoderConfig& video_config) {}

  static base::Optional<MediaTypeRestriction> GetPlatformRestrictionForType(
      const std::string& type);
  static void SetMediaCodecCapability(const std::string& codec_info);
  static Json::Value SupportedCodec() { return supported_codec_; }

 protected:
  virtual ~MediaPlatformAPI();

  class BufferQueue {
   public:
    BufferQueue();
    void Push(const scoped_refptr<DecoderBuffer>& buffer, FeedType type);
    const std::pair<scoped_refptr<DecoderBuffer>, FeedType> Front();
    void Pop();
    void Clear();

    bool Empty();
    size_t DataSize() const;

   private:
    using DecoderBufferPair =
        std::pair<scoped_refptr<DecoderBuffer>, MediaPlatformAPI::FeedType>;
    using DecoderBufferQueue = std::queue<DecoderBufferPair>;
    DecoderBufferQueue queue_;
    size_t data_size_;

    DISALLOW_COPY_AND_ASSIGN(BufferQueue);
  };

  std::unique_ptr<BufferQueue> buffer_queue_;

 private:
  friend class base::RefCountedThreadSafe<MediaPlatformAPI>;

  static Json::Value supported_codec_;

  DISALLOW_COPY_AND_ASSIGN(MediaPlatformAPI);
};

}  // namespace media

#endif  // MEDIA_BASE_NEVA_MEDIA_PLATFORM_API_H
