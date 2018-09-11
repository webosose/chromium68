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

#ifndef MEDIA_RENDERERS_NEVA_MEDIA_PLAYER_RENDERER_FACTORY_H_
#define MEDIA_RENDERERS_NEVA_MEDIA_PLAYER_RENDERER_FACTORY_H_

#include <memory>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "media/base/media_export.h"
#include "media/base/renderer_factory.h"

namespace media {

class AudioDecoder;
class AudioHardwareConfig;
class AudioRendererSink;
class DecoderFactory;
class GpuVideoAcceleratorFactories;
class MediaLog;
class VideoDecoder;
class VideoRendererSink;
class MediaPlatformAPI;

using CreateAudioDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<AudioDecoder>>()>;
using CreateVideoDecodersCB =
    base::RepeatingCallback<std::vector<std::unique_ptr<VideoDecoder>>()>;

class MEDIA_EXPORT NevaMediaPlayerRendererFactory : public RendererFactory {
 public:
  using GetGpuFactoriesCB = base::Callback<GpuVideoAcceleratorFactories*()>;

  static bool Enabled();

  NevaMediaPlayerRendererFactory(MediaLog* media_log,
                      DecoderFactory* decoder_factory,
                      const GetGpuFactoriesCB& get_gpu_factories_cb);
  ~NevaMediaPlayerRendererFactory() final;

  std::unique_ptr<Renderer> CreateRenderer(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const scoped_refptr<base::TaskRunner>& worker_task_runner,
      AudioRendererSink* audio_renderer_sink,
      VideoRendererSink* video_renderer_sink,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space) final;

  void SetMediaPlatformAPI(
      const scoped_refptr<MediaPlatformAPI>& media_platform_api) override;

 private:
  std::vector<std::unique_ptr<AudioDecoder>> CreateAudioDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner);
  std::vector<std::unique_ptr<VideoDecoder>> CreateVideoDecoders(
      const scoped_refptr<base::SingleThreadTaskRunner>& media_task_runner,
      const RequestOverlayInfoCB& request_overlay_info_cb,
      const gfx::ColorSpace& target_color_space,
      GpuVideoAcceleratorFactories* gpu_factories);

  MediaLog* media_log_;

  // Factory to create extra audio and video decoders.
  // Could be nullptr if not extra decoders are available.
  DecoderFactory* decoder_factory_;

  // Creates factories for supporting video accelerators. May be null.
  GetGpuFactoriesCB get_gpu_factories_cb_;

  scoped_refptr<MediaPlatformAPI> media_platform_api_;

  DISALLOW_COPY_AND_ASSIGN(NevaMediaPlayerRendererFactory);
};

}  // namespace media

#endif  // MEDIA_RENDERERS_NEVA_MEDIA_PLAYER_RENDERER_FACTORY_H_
