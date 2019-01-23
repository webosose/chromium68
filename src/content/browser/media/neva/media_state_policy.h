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

#ifndef CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_POLICY_H
#define CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_POLICY_H

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

// MediaStatePolicy is responsible for answering following two questions:
// 1) Who should be suspended.
// 2) Who should be activated.
// If logical decision is done, then MediaStatePolicy requests execution
// to MediaStatePolicy::Client.
class CONTENT_EXPORT MediaStatePolicy {
 public:
  class Client {
   public:
    virtual void PermitMediaActivation(RenderFrameHost* render_frame_host,
                                       int player_id) = 0;
    virtual void SuspendMedia(RenderFrameHost* render_frame_host,
                              int player_id) = 0;
  };

  MediaStatePolicy(Client* client);
  virtual ~MediaStatePolicy();

  virtual void Initialize() = 0;
  virtual void OnMediaActivated(RenderFrameHost* render_frame_host,
                                int player_id) = 0;
  virtual void OnMediaActivationRequested(RenderFrameHost* render_frame_host,
                                          int player_id) = 0;
  virtual void OnMediaCreated(RenderFrameHost* render_frame_host,
                              int player_id,
                              bool will_use_media_resource) = 0;
  virtual void OnMediaDestroyed(RenderFrameHost* render_frame_host,
                                int player_id) = 0;
  virtual void OnMediaResumeRequested(WebContents* web_contents) = 0;
  virtual void OnMediaResumeRequested(RenderFrameHost* render_frame_host) = 0;
  virtual void OnMediaSuspended(RenderFrameHost* render_frame_host,
                                int player_id) = 0;
  virtual void OnMediaSuspendRequested(WebContents* web_contents) = 0;
  virtual void OnMediaSuspendRequested(RenderFrameHost* render_frame_host) = 0;
  virtual void OnRenderFrameDeleted(RenderFrameHost* render_frame_host) = 0;

 protected:
  Client* client_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_POLICY_H
