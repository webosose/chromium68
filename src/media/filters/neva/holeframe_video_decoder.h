// Copyright (c) 2017 LG Electronics, Inc.

#ifndef MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_
#define MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_

#include <list>

#include "base/callback.h"
#include "media/base/video_decoder.h"
#include "media/base/video_decoder_config.h"

namespace base {
class SingleThreadTaskRunner;
}

namespace media {

class DecoderBuffer;

class MEDIA_EXPORT HoleFrameVideoDecoder : public VideoDecoder {
 public:
  HoleFrameVideoDecoder(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner);
  ~HoleFrameVideoDecoder() override;

  // VideoDecoder implementation.
  std::string GetDisplayName() const override;
  void Decode(scoped_refptr<DecoderBuffer> buffer,
              const DecodeCB& decode_cb) override;
  void Reset(const base::Closure& closure) override;

  virtual bool FeedForPlatformMediaVideoDecoder(
      const scoped_refptr<DecoderBuffer>& buffer) = 0;

 protected:
  enum DecoderState {
    kUninitialized,
    kNormal,
    kDecodeFinished,
    kError
  };

  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;

  DecoderState state_;

  OutputCB output_cb_;

  VideoDecoderConfig config_;

  DISALLOW_COPY_AND_ASSIGN(HoleFrameVideoDecoder);
};

}  // namespace media

#endif  // MEDIA_FILTERS_NEVA_HOLEFRAME_VIDEO_DECODER_H_
