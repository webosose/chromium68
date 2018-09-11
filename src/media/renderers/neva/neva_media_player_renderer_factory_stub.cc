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

#include "media/renderers/neva/neva_media_player_renderer_factory.h"

#include "media/base/audio_decoder.h"
#include "media/base/video_decoder.h"
#include "media/base/neva/media_platform_api.h"

namespace media {

NevaMediaPlayerRendererFactory::NevaMediaPlayerRendererFactory(
    MediaLog* media_log,
    DecoderFactory* decoder_factory,
    const GetGpuFactoriesCB& get_gpu_factories_cb) {
  // These assignments just for eliminating compile warning.
  media_log_ = media_log;
  decoder_factory_ = decoder_factory;
}

NevaMediaPlayerRendererFactory::~NevaMediaPlayerRendererFactory() {
}

bool NevaMediaPlayerRendererFactory::Enabled() {
  return false;
}

std::unique_ptr<Renderer> NevaMediaPlayerRendererFactory::CreateRenderer(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const scoped_refptr<base::TaskRunner>& worker_task_runner,
    AudioRendererSink* audio_renderer_sink,
    VideoRendererSink* video_renderer_sink,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space) {
  return nullptr;
}

std::vector<std::unique_ptr<AudioDecoder>>
NevaMediaPlayerRendererFactory::CreateAudioDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner) {
  // This function is not used.
  return std::vector<std::unique_ptr<AudioDecoder>>();
}

std::vector<std::unique_ptr<VideoDecoder>>
NevaMediaPlayerRendererFactory::CreateVideoDecoders(
    const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
    const RequestOverlayInfoCB& request_overlay_info_cb,
    const gfx::ColorSpace& target_color_space,
    GpuVideoAcceleratorFactories* gpu_factories) {
  // This function is not used.
  return std::vector<std::unique_ptr<VideoDecoder>>();
}

void NevaMediaPlayerRendererFactory::SetMediaPlatformAPI(
    const scoped_refptr<MediaPlatformAPI>& media_platform_api) {
}

}  // namespace media
