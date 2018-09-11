// Copyright (c) 2017 LG Electronics, Inc.

#include "media/filters/neva/emptybuffer_audio_decoder.h"

#include "base/callback_helpers.h"
#include "base/single_thread_task_runner.h"
#include "media/base/audio_buffer.h"
#include "media/base/audio_decoder_config.h"
#include "media/base/audio_discard_helper.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/decoder_buffer.h"
#include "media/base/sample_format.h"

namespace media {

EmptyBufferAudioDecoder::EmptyBufferAudioDecoder(
    const scoped_refptr<base::SingleThreadTaskRunner>& task_runner)
    : task_runner_(task_runner),
      state_(kUninitialized),
      av_sample_format_(0) {
}

EmptyBufferAudioDecoder::~EmptyBufferAudioDecoder() {
  DCHECK(task_runner_->BelongsToCurrentThread());

  if (state_ != kUninitialized) {
    ResetTimestampState();
  }
}

std::string EmptyBufferAudioDecoder::GetDisplayName() const {
  return "EmptyBufferAudioDecoder";
}

void EmptyBufferAudioDecoder::Decode(scoped_refptr<DecoderBuffer> buffer,
                                     const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK(!decode_cb.is_null());
  CHECK_NE(state_, kUninitialized);
  DecodeCB decode_cb_bound = BindToCurrentLoop(decode_cb);

  if (state_ == kError) {
    decode_cb_bound.Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  // Do nothing if decoding has finished.
  if (state_ == kDecodeFinished) {
    decode_cb_bound.Run(DecodeStatus::OK);
    return;
  }

  DecodeBuffer(buffer, decode_cb_bound);
}

void EmptyBufferAudioDecoder::Reset(const base::Closure& closure) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  state_ = kNormal;
  ResetTimestampState();
  task_runner_->PostTask(FROM_HERE, closure);
}

void EmptyBufferAudioDecoder::DecodeBuffer(
    const scoped_refptr<DecoderBuffer>& buffer,
    const DecodeCB& decode_cb) {
  DCHECK(task_runner_->BelongsToCurrentThread());
  DCHECK_NE(state_, kUninitialized);
  DCHECK_NE(state_, kDecodeFinished);
  DCHECK_NE(state_, kError);
  DCHECK(buffer);

  // Make sure we are notified if http://crbug.com/49709 returns.  Issue also
  // occurs with some damaged files.
  if (!buffer->end_of_stream() && buffer->timestamp() == kNoTimestamp) {
    DVLOG(1) << "Received a buffer without timestamps!";
    decode_cb.Run(DecodeStatus::DECODE_ERROR);
    return;
  }

  bool has_produced_frame;
  do {
    has_produced_frame = false;
    if (buffer->data() && buffer->data_size() > 0) {
      if (!FeedForPlatformMediaAudioDecoder(buffer)) {
        state_ = kError;
        decode_cb.Run(DecodeStatus::DECODE_ERROR);
        return;
      }
      has_produced_frame = true;

      scoped_refptr<AudioBuffer> audio_buffer;
      int frame_count = buffer->duration().InMicroseconds() *
        config_.samples_per_second() / base::Time::kMicrosecondsPerSecond;

      audio_buffer = AudioBuffer::CreateEmptyBuffer(
          config_.channel_layout(),
          ChannelLayoutToChannelCount(config_.channel_layout()),
          config_.samples_per_second(),
          frame_count,
          buffer->timestamp());

      output_cb_.Run(audio_buffer);
    }
    // Repeat to flush the decoder after receiving EOS buffer.
  } while (buffer->end_of_stream() && has_produced_frame);

  if (buffer->end_of_stream()) {
    if (!FeedForPlatformMediaAudioDecoder(buffer)) {
      state_ = kError;
      decode_cb.Run(DecodeStatus::DECODE_ERROR);
      return;
    }
    state_ = kDecodeFinished;
  }

  decode_cb.Run(DecodeStatus::OK);
}

bool EmptyBufferAudioDecoder::ConfigureDecoder() {
  if (!config_.IsValidConfig()) {
    DLOG(ERROR) << "Invalid audio stream -"
                << " codec: " << config_.codec()
                << " channel layout: " << config_.channel_layout()
                << " bits per channel: " << config_.bits_per_channel()
                << " samples per second: " << config_.samples_per_second();
    return false;
  }

  if (config_.is_encrypted()) {
    DLOG(ERROR) << "Encrypted audio stream not supported";
    return false;
  }

  discard_helper_.reset(
      new AudioDiscardHelper(config_.samples_per_second(),
                             config_.codec_delay(), false));
  ChannelLayoutToChannelCount(config_.channel_layout());
  ResetTimestampState();
  return true;
}

void EmptyBufferAudioDecoder::ResetTimestampState() {
  discard_helper_->Reset(config_.codec_delay());
}

}  // namespace media
