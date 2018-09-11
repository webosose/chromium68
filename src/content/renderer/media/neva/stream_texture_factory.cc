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

#include "content/renderer/media/neva/stream_texture_factory.h"

#include "base/macros.h"
#include "components/viz/common/gpu/context_provider.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "ui/gfx/geometry/size.h"

namespace content {

StreamTextureProxy::StreamTextureProxy(std::unique_ptr<StreamTextureHost> host)
    : host_(std::move(host)), client_(nullptr) {}

StreamTextureProxy::~StreamTextureProxy() {}

void StreamTextureProxy::Release() {
  {
    // Cannot call into |client_| anymore (from any thread) after returning
    // from here.
    base::AutoLock lock(lock_);
    client_ = NULL;
  }
  // Release is analogous to the destructor, so there should be no more external
  // calls to this object in Release. Therefore there is no need to acquire the
  // lock to access |loop_|.
  if (!loop_.get() || loop_->BelongsToCurrentThread() ||
      !loop_->DeleteSoon(FROM_HERE, this)) {
    delete this;
  }
}

void StreamTextureProxy::BindToLoop(
    int32_t stream_id,
    cc::VideoFrameProvider::Client* client,
    scoped_refptr<base::SingleThreadTaskRunner> loop) {
  DCHECK(loop.get());

  {
    base::AutoLock lock(lock_);
    DCHECK(!loop_.get() || (loop.get() == loop_.get()));
    loop_ = loop;
    client_ = client;
  }

  if (loop->BelongsToCurrentThread()) {
    BindOnThread(stream_id);
    return;
  }
  // Unretained is safe here only because the object is deleted on |loop_|
  // thread.
  loop->PostTask(FROM_HERE, base::Bind(&StreamTextureProxy::BindOnThread,
                                       base::Unretained(this), stream_id));
}

void StreamTextureProxy::BindOnThread(int32_t stream_id) {
  host_->BindToCurrentThread(stream_id, this);
}

void StreamTextureProxy::OnFrameAvailable() {
  base::AutoLock lock(lock_);
  if (client_)
    client_->DidReceiveFrame();
}

// static
scoped_refptr<StreamTextureFactory> StreamTextureFactory::Create(
    scoped_refptr<ui::ContextProviderCommandBuffer> context_provider) {
  return new StreamTextureFactory(std::move(context_provider));
}

StreamTextureFactory::StreamTextureFactory(
    scoped_refptr<ui::ContextProviderCommandBuffer> context_provider)
    : context_provider_(std::move(context_provider)),
      channel_(context_provider_->GetCommandBufferProxy()->channel()) {
  DCHECK(channel_);
}

StreamTextureFactory::~StreamTextureFactory() {}

media::StreamTextureProxyInterface* StreamTextureFactory::CreateProxy() {
  return new StreamTextureProxy(std::make_unique<StreamTextureHost>(channel_));
}

unsigned StreamTextureFactory::CreateStreamTexture(
    unsigned texture_target,
    unsigned* texture_id,
    gpu::Mailbox* texture_mailbox) {
  GLuint stream_id = 0;
  gpu::gles2::GLES2Interface* gl = context_provider_->ContextGL();
  gl->GenTextures(1, texture_id);
  gl->ShallowFlushCHROMIUM();
  stream_id = context_provider_->GetCommandBufferProxy()->CreateStreamTexture(
      *texture_id);

  if (stream_id) {
    gl->GenMailboxCHROMIUM(texture_mailbox->name);
    gl->ProduceTextureDirectCHROMIUM(*texture_id, texture_mailbox->name);
  } else {
    gl->DeleteTextures(1, texture_id);
    *texture_id = 0;
  }
  return stream_id;
}

void StreamTextureFactory::SetStreamTextureActiveRegion(
    int32_t stream_id,
    const gfx::Rect& region) {
  channel_->Send(new GpuStreamTextureMsg_SetActiveRegion(stream_id, region));
}

gpu::gles2::GLES2Interface* StreamTextureFactory::ContextGL() {
  return context_provider_->ContextGL();
}

}  // namespace content
