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

#include "content/browser/media/neva/media_state_manager_impl.h"

#include "base/memory/singleton.h"
#include "content/browser/media/neva/media_state_policy_factory.h"

namespace content {

MediaStateManager* MediaStateManager::GetInstance() {
  return MediaStateManagerImpl::GetInstance();
}

MediaStateManagerImpl* MediaStateManagerImpl::GetInstance() {
  return base::Singleton<MediaStateManagerImpl>::get();
}

MediaStateManagerImpl::MediaStateManagerImpl() {
  policy_.reset(MediaStatePolicyFactory::CreateMediaStatePolicy(this));
}

MediaStateManagerImpl::~MediaStateManagerImpl() {
  policy_.reset(nullptr);
}

void MediaStateManagerImpl::OnMediaActivated(RenderFrameHost* render_frame_host,
                                             int player_id) {
  policy_->OnMediaActivated(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaActivationRequested(
    RenderFrameHost* render_frame_host,
    int player_id) {
  policy_->OnMediaActivationRequested(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaCreated(RenderFrameHost* render_frame_host,
                                           int player_id) {
  policy_->OnMediaCreated(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaDestroyed(RenderFrameHost* render_frame_host,
                                             int player_id) {
  policy_->OnMediaDestroyed(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnMediaSuspended(RenderFrameHost* render_frame_host,
                                             int player_id) {
  policy_->OnMediaSuspended(render_frame_host, player_id);
}

void MediaStateManagerImpl::OnRenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  policy_->OnRenderFrameDeleted(render_frame_host);
}

void MediaStateManagerImpl::OnWebContentsDestroyed(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    OnRenderFrameDeleted(rfh);
}

void MediaStateManagerImpl::ResumeAllMedia(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    rfh->SetSuppressed(false);
  policy_->OnMediaResumeRequested(web_contents);
}

void MediaStateManagerImpl::ResumeAllMedia(RenderFrameHost* render_frame_host) {
  render_frame_host->SetSuppressed(false);
  policy_->OnMediaResumeRequested(render_frame_host);
}

void MediaStateManagerImpl::SuspendAllMedia(WebContents* web_contents) {
  for (auto* rfh : web_contents->GetAllFrames())
    rfh->SetSuppressed(true);
  policy_->OnMediaSuspendRequested(web_contents);
}

void MediaStateManagerImpl::SuspendAllMedia(
    RenderFrameHost* render_frame_host) {
  render_frame_host->SetSuppressed(true);
  policy_->OnMediaSuspendRequested(render_frame_host);
}

void MediaStateManagerImpl::PermitMediaActivation(
    RenderFrameHost* render_frame_host,
    int player_id) {
  render_frame_host->PermitMediaActivation(player_id);
}

void MediaStateManagerImpl::SuspendMedia(RenderFrameHost* render_frame_host,
                                         int player_id) {
  render_frame_host->SuspendMedia(player_id);
}

}  // namespace content
