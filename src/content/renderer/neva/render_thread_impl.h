// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef CONTENT_RENDERER_NEVA_RENDER_THREAD_IMPL_H_
#define CONTENT_RENDERER_NEVA_RENDER_THREAD_IMPL_H_

#include <stdio.h>
#include <type_traits>
#include "components/viz/common/gpu/context_provider.h"
#include "content/renderer/media/neva/stream_texture_factory.h"
#include "gpu/ipc/client/command_buffer_proxy_impl.h"

namespace content {
class StreamTextureFactory;

namespace neva {

template <typename original_t>
class RenderThreadImpl {
 public:
  scoped_refptr<media::StreamTextureFactoryInterface> GetStreamTextureFactory();

 protected:
  scoped_refptr<media::StreamTextureFactoryInterface> stream_texture_factory_;
};

template <typename original_t>
scoped_refptr<media::StreamTextureFactoryInterface>
RenderThreadImpl<original_t>::GetStreamTextureFactory() {
  original_t* self(static_cast<original_t*>(this));
  scoped_refptr<media::StreamTextureFactoryInterface> stream_texture_factory =
      self->stream_texture_factory_;
  DCHECK(self->IsMainThread());

  if (!stream_texture_factory.get() ||
      stream_texture_factory->ContextGL()->GetGraphicsResetStatusKHR() !=
          GL_NO_ERROR) {
    scoped_refptr<ui::ContextProviderCommandBuffer> shared_context_provider =
        self->SharedMainThreadContextProvider();
    if (!shared_context_provider) {
      stream_texture_factory = nullptr;
      return nullptr;
    }
    DCHECK(shared_context_provider->GetCommandBufferProxy());
    DCHECK(shared_context_provider->GetCommandBufferProxy()->channel());
    stream_texture_factory =
        StreamTextureFactory::Create(std::move(shared_context_provider));
  }
  return stream_texture_factory;
}

}  // namespace neva
}  // namespace content

#endif  // CONTENT_RENDERER_NEVA_RENDER_THREAD_IMPL_H_
