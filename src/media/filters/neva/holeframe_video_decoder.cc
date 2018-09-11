// Copyright (c) 2017 LG Electronics, Inc.

#include "media/filters/neva/holeframe_video_decoder.h"

#include <algorithm>
#include <string>

#include "base/bind.h"
#include "base/callback_helpers.h"
#include "base/single_thread_task_runner.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/pipeline.h"
#include "media/base/video_decoder_config.h"
#include "media/base/video_frame.h"
#include "media/base/video_util.h"

namespace media {

HoleFrameVideoDecoder::HoleFrameVideoDecoder(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : task_runner_(task_runner),
      state_(kUninitialized) {
}

HoleFrameVideoDecoder::~HoleFrameVideoDecoder() {
  DCHECK(task_runner_->BelongsToCurrentThread());
}

std::string HoleFrameVideoDecoder::GetDisplayName() const {
  return "HoleFrameVideoDecoder";
}

void HoleFrameVideoDecoder::Decode(scoped_refptr<DecoderBuffer> buffer,
                                   const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(buffer);
  DCHECK(!decode_cb.is_null());
  CHECK_NE(state_, kUninitialized);

  DecodeCB decode_cb_bound = BindToCurrentLoop(decode_cb);

  if (state_ == kError) {
    decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  if (state_ == kDecodeFinished) {
    decode_cb_bound.Run(DecodeStatus::OK);
    return;
  }

  DCHECK_EQ(state_, kNormal);

  // During decode, because reads are issued asynchronously, it is possible to
  // receive multiple end of stream buffers since each decode is acked. When the
  // first end of stream buffer is read, FFmpeg may still have frames queued
  // up in the decoder so we need to go through the decode loop until it stops
  // giving sensible data.  After that, the decoder should output empty
  // frames.  There are three states the decoder can be in:
  //
  //   kNormal: This is the starting state. Buffers are decoded. Decode errors
  //            are discarded.
  //   kDecodeFinished: All calls return empty frames.
  //   kError: Unexpected error happened.
  //
  // These are the possible state transitions.
  //
  // kNormal -> kDecodeFinished:
  //     When EOS buffer is received and the codec has been flushed.
  // kNormal -> kError:
  //     A decoding error occurs and decoding needs to stop.
  // (any state) -> kNormal:
  //     Any time Reset() is called.

  bool has_produced_frame;
  do {
    has_produced_frame = false;
    if (buffer->data() && buffer->data_size() > 0) {
      if (!FeedForPlatformMediaVideoDecoder(buffer)) {
        state_ = kError;
        decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
        return;
      }
      has_produced_frame = true;

      scoped_refptr<media::VideoFrame> video_frame =
#if defined(VIDEO_HOLE)
        media::VideoFrame::CreateHoleFrame(config_.natural_size());
#else
        media::VideoFrame::CreateTransparentFrame(config_.natural_size());
#endif

      video_frame->set_timestamp(buffer->timestamp());

      output_cb_.Run(video_frame);
    }
    // Repeat to flush the decoder after receiving EOS buffer.
  } while (buffer->end_of_stream() && has_produced_frame);

  if (buffer->end_of_stream()) {
    if (!FeedForPlatformMediaVideoDecoder(buffer)) {
      state_ = kError;
      decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
      return;
    }
    state_ = kDecodeFinished;
  }

  decode_cb_bound.Run(DecodeStatus::OK);
}

void HoleFrameVideoDecoder::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  state_ = kNormal;
  task_runner_->PostTask(FROM_HERE, closure);
}

}  // namespace media
