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

#ifndef CONTENT_RENDERER_MEDIA_NEVA_STREAM_TEXTURE_FACTORY_H_
#define CONTENT_RENDERER_MEDIA_NEVA_STREAM_TEXTURE_FACTORY_H_

#include <stdint.h>

#include <memory>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/message_loop/message_loop.h"
#include "base/single_thread_task_runner.h"
#include "cc/layers/video_frame_provider.h"
#include "content/renderer/gpu/neva/stream_texture_host.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "media/blink/neva/stream_texture_interface.h"
#include "services/ui/public/cpp/gpu/context_provider_command_buffer.h"
#include "ui/gfx/geometry/rect.h"

namespace gpu {
namespace gles2 {
class GLES2Interface;
}  // namespace gles2
class GpuChannelHost;
}  // namespace gpu

namespace ui {
class ContextProviderCommandBuffer;
}  // namespace ui

namespace content {

class StreamTextureFactory;

// The proxy class for the gpu thread to notify the compositor thread
// when a new video frame is available.
class StreamTextureProxy : public StreamTextureHost::Listener,
                           public media::StreamTextureProxyInterface {
 public:
  explicit StreamTextureProxy(std::unique_ptr<StreamTextureHost> host);
  ~StreamTextureProxy() override;

  // Initialize and bind to the loop, which becomes the thread that
  // a connected client will receive callbacks on. This can be called
  // on any thread, but must be called with the same loop every time.
  void BindToLoop(int32_t stream_id,
                  cc::VideoFrameProvider::Client* client,
                  scoped_refptr<base::SingleThreadTaskRunner> loop) override;

  // StreamTextureHost::Listener implementation:
  void OnFrameAvailable() override;

 private:
  friend class StreamTextureFactory;

  void BindOnThread(int32_t stream_id);
  void Release() override;

  const std::unique_ptr<StreamTextureHost> host_;

  // Protects access to |client_| and |loop_|.
  base::Lock lock_;
  cc::VideoFrameProvider::Client* client_;
  scoped_refptr<base::SingleThreadTaskRunner> loop_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureProxy);
};

// Factory class for managing stream textures.
class StreamTextureFactory : public media::StreamTextureFactoryInterface {
 public:
  static scoped_refptr<StreamTextureFactory> Create(
      scoped_refptr<ui::ContextProviderCommandBuffer> context_provider);

  // Create the StreamTextureProxy object.
  media::StreamTextureProxyInterface* CreateProxy() override;

  // Creates a gpu::StreamTexture and returns its id.  Sets |*texture_id| to the
  // client-side id of the gpu::StreamTexture. The texture is produced into
  // a mailbox so it can be shipped in a VideoFrame.
  unsigned CreateStreamTexture(unsigned texture_target,
                               unsigned* texture_id,
                               gpu::Mailbox* texture_mailbox) override;

  // Set the streamTexture active region for the given stream Id.
  void SetStreamTextureActiveRegion(int32_t stream_id,
                                    const gfx::Rect& region) override;

  gpu::gles2::GLES2Interface* ContextGL() override;

 private:
  StreamTextureFactory(
      scoped_refptr<ui::ContextProviderCommandBuffer> context_provider);
  ~StreamTextureFactory() override;

  scoped_refptr<ui::ContextProviderCommandBuffer> context_provider_;
  scoped_refptr<gpu::GpuChannelHost> channel_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureFactory);
};

}  // namespace content

#endif  // CONTENT_RENDERER_MEDIA_NEVA_STREAM_TEXTURE_FACTORY_H_
