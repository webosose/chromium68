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

#ifndef CONTENT_RENDERER_GPU_NEVA_STREAM_TEXTURE_HOST_H_
#define CONTENT_RENDERER_GPU_NEVA_STREAM_TEXTURE_HOST_H_

#include <stdint.h>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ipc/ipc_listener.h"
#include "ipc/ipc_message.h"

namespace gfx {
class Size;
}

namespace gpu {
class GpuChannelHost;
}

namespace content {

// Class for handling all the IPC messages between the GPU process and
// StreamTextureProxy.
class StreamTextureHost : public IPC::Listener {
 public:
  explicit StreamTextureHost(scoped_refptr<gpu::GpuChannelHost> channel);
  ~StreamTextureHost() override;

  // Listener class that is listening to the stream texture updates. It is
  // implemented by StreamTextureProxyImpl.
  class Listener {
   public:
    virtual void OnFrameAvailable() = 0;
    virtual ~Listener() {}
  };

  bool BindToCurrentThread(int32_t stream_id, Listener* listener);

  // IPC::Channel::Listener implementation:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnChannelError() override;

 private:
  // Message handlers:
  void OnFrameAvailable();

  int stream_id_;
  Listener* listener_;
  scoped_refptr<gpu::GpuChannelHost> channel_;
  base::WeakPtrFactory<StreamTextureHost> weak_ptr_factory_;

  DISALLOW_IMPLICIT_CONSTRUCTORS(StreamTextureHost);
};

}  // namespace content

#endif  // CONTENT_RENDERER_GPU_NEVA_STREAM_TEXTURE_HOST_H_
