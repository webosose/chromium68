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

#include "content/renderer/gpu/neva/stream_texture_host.h"

#include "content/renderer/render_thread_impl.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/ipc/common/gpu_messages.h"
#include "ipc/ipc_message_macros.h"

namespace content {

StreamTextureHost::StreamTextureHost(scoped_refptr<gpu::GpuChannelHost> channel)
    : stream_id_(0),
      listener_(NULL),
      channel_(std::move(channel)),
      weak_ptr_factory_(this) {
  DCHECK(channel_);
}

StreamTextureHost::~StreamTextureHost() {
  if (channel_.get() && stream_id_)
    channel_->RemoveRoute(stream_id_);
}

bool StreamTextureHost::BindToCurrentThread(int32_t stream_id,
                                            Listener* listener) {
  listener_ = listener;
  if (channel_.get() && stream_id && !stream_id_) {
    stream_id_ = stream_id;
    channel_->AddRoute(stream_id, weak_ptr_factory_.GetWeakPtr());
    channel_->Send(new GpuStreamTextureMsg_StartListening(stream_id));
    return true;
  }

  return false;
}

bool StreamTextureHost::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(StreamTextureHost, message)
  IPC_MESSAGE_HANDLER(GpuStreamTextureMsg_FrameAvailable, OnFrameAvailable);
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  DCHECK(handled);
  return handled;
}

void StreamTextureHost::OnChannelError() {}

void StreamTextureHost::OnFrameAvailable() {
  if (listener_)
    listener_->OnFrameAvailable();
}

}  // namespace content
