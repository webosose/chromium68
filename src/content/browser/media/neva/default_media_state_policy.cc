// Copyright (c) 2019 LG Electronics, Inc.
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

#include "content/browser/media/neva/default_media_state_policy.h"

#include <algorithm>

#include "content/public/browser/render_frame_host.h"

namespace content {

DefaultMediaStatePolicy::DefaultMediaStatePolicy(Client* client)
    : MediaStatePolicy(client) {}

void DefaultMediaStatePolicy::Initialize() {}

void DefaultMediaStatePolicy::OnMediaActivated(
    RenderFrameHost* render_frame_host,
    int player_id) {}

void DefaultMediaStatePolicy::OnMediaActivationRequested(
    RenderFrameHost* render_frame_host,
    int player_id) {
  if (!client_)
    return;

  client_->PermitMediaActivation(render_frame_host, player_id);
}

void DefaultMediaStatePolicy::OnMediaCreated(RenderFrameHost* render_frame_host,
                                             int player_id,
                                             bool will_use_media_resource) {
  player_list_[render_frame_host].insert(player_id);
}

void DefaultMediaStatePolicy::OnMediaDestroyed(
    RenderFrameHost* render_frame_host,
    int player_id) {
  player_list_[render_frame_host].erase(player_id);
}

void DefaultMediaStatePolicy::OnMediaResumeRequested(
    WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    OnMediaResumeRequested(rfh);
}

void DefaultMediaStatePolicy::OnMediaResumeRequested(
    RenderFrameHost* render_frame_host) {
  if (!client_)
    return;

  for (auto iter = player_list_[render_frame_host].begin();
       iter != player_list_[render_frame_host].end(); iter++) {
    client_->PermitMediaActivation(render_frame_host, *iter);
  }
}

void DefaultMediaStatePolicy::OnMediaSuspended(
    RenderFrameHost* render_frame_host,
    int player_id) {}

void DefaultMediaStatePolicy::OnMediaSuspendRequested(
    WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    OnMediaSuspendRequested(rfh);
}

void DefaultMediaStatePolicy::OnMediaSuspendRequested(
    RenderFrameHost* render_frame_host) {
  if (!client_)
    return;

  for (auto iter = player_list_[render_frame_host].begin();
       iter != player_list_[render_frame_host].end(); iter++) {
    client_->SuspendMedia(render_frame_host, *iter);
  }
}

void DefaultMediaStatePolicy::OnRenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  player_list_.erase(render_frame_host);
}

}  // namespace content
