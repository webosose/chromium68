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

#include "remote_pal_observer.h"

#include "content/public/renderer/render_frame.h"
#include "content/renderer/render_view_impl.h"
#include "remote_pal_ipc/remote_pal_observers_gen.h"

namespace content {

RemotePalObserver::RemotePalObserver(RenderViewImpl* render_view)
    : RenderViewObserver(render_view) {
  InitializeInterfaceObservers();
}

RemotePalObserver::~RemotePalObserver() {}

bool RemotePalObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(RemotePalObserver, message)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void RemotePalObserver::OnDestruct() {
  delete this;
}

void RemotePalObserver::InitializeInterfaceObservers() {
  RenderFrame* mainRenderFrame = render_view()->GetMainRenderFrame();
  if (!mainRenderFrame) {
    LOG(ERROR) << "Failed to create dispatcher because MainRenderFrame is NULL";
    return;
  }
  #include "remote_pal_ipc/remote_pal_observers_gen_impl.cc"
}

}  // namespace content
