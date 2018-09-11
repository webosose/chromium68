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

#ifndef MEDIA_BLINK_NEVA_STREAM_TEXTURE_INTERFACE_H_
#define MEDIA_BLINK_NEVA_STREAM_TEXTURE_INTERFACE_H_
#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "cc/layers/video_frame_provider.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "ui/gfx/geometry/rect.h"

namespace media {

class StreamTextureProxyInterface;

class StreamTextureFactoryInterface
    : public base::RefCounted<StreamTextureFactoryInterface> {
 public:
  // Create the StreamTextureProxy object.
  virtual StreamTextureProxyInterface* CreateProxy() = 0;

  // Creates a gpu::StreamTexture and returns its id.  Sets |*texture_id| to the
  // client-side id of the gpu::StreamTexture. The texture is produced into
  // a mailbox so it can be shipped in a VideoFrame.
  virtual unsigned CreateStreamTexture(unsigned texture_target,
                                       unsigned* texture_id,
                                       gpu::Mailbox* texture_mailbox) = 0;

  // Set the streamTexture active region for the given stream Id.
  virtual void SetStreamTextureActiveRegion(int32_t stream_id,
                                            const gfx::Rect& region) = 0;

  virtual gpu::gles2::GLES2Interface* ContextGL() = 0;

 protected:
  virtual ~StreamTextureFactoryInterface() {}

 private:
  friend class base::RefCounted<StreamTextureFactoryInterface>;
};

typedef base::RepeatingCallback<scoped_refptr<StreamTextureFactoryInterface>()>
    StreamTextureFactoryCreateCB;

class StreamTextureProxyInterface {
 public:
  // Initialize and bind to the loop, which becomes the thread that
  // a connected client will receive callbacks on. This can be called
  // on any thread, but must be called with the same loop every time.
  virtual void BindToLoop(int32_t stream_id,
                          cc::VideoFrameProvider::Client* client,
                          scoped_refptr<base::SingleThreadTaskRunner> loop) = 0;
  struct Deleter {
    inline void operator()(StreamTextureProxyInterface* ptr) const {
      ptr->Release();
    }
  };

 protected:
  virtual ~StreamTextureProxyInterface(){};
  virtual void Release() = 0;
};

typedef std::unique_ptr<StreamTextureProxyInterface,
                        StreamTextureProxyInterface::Deleter>
    ScopedStreamTextureProxy;

}  // namespace media

#endif  // MEDIA_BLINK_NEVA_STREAM_TEXTURE_INTERFACE_H_
