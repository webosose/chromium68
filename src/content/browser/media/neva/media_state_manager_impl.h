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

#ifndef CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_MANAGER_IMPL_H
#define CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_MANAGER_IMPL_H

#include "base/memory/singleton.h"
#include "content/browser/media/neva/media_state_policy.h"
#include "content/public/browser/neva/media_state_manager.h"

namespace content {

// This class is implementation of MediaStateManager.
// MediaStateManagerImpl receives messages from each components and executes
// action. 'Logical decision' is not resposibility of MediaStateManagerImpl.
// It will be done by MediaStatePolicy.
class CONTENT_EXPORT MediaStateManagerImpl : public MediaStateManager,
                                             public MediaStatePolicy::Client {
 public:
  static MediaStateManagerImpl* GetInstance();

  // MediaStateManager implementation.
  void OnMediaActivated(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaActivationRequested(RenderFrameHost* render_frame_host,
                                  int player_id) override;
  void OnMediaCreated(RenderFrameHost* render_frame_host,
                      int player_id,
                      bool will_use_media_resource) override;
  void OnMediaDestroyed(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaSuspended(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnRenderFrameDeleted(RenderFrameHost* render_frame_host) override;
  void OnWebContentsDestroyed(WebContents* web_contents) override;
  void ResumeAllMedia(WebContents* web_contents) override;
  void ResumeAllMedia(RenderFrameHost* render_frame_host) override;
  void SuspendAllMedia(WebContents* web_contents) override;
  void SuspendAllMedia(RenderFrameHost* render_frame_host) override;

  // MediaStatePolicy::Client implementation.
  void PermitMediaActivation(RenderFrameHost* render_frame_host,
                             int player_id) override;
  void SuspendMedia(RenderFrameHost* render_frame_host, int player_id) override;

 private:
  friend struct base::DefaultSingletonTraits<MediaStateManagerImpl>;
  MediaStateManagerImpl();
  ~MediaStateManagerImpl() override;

  std::unique_ptr<MediaStatePolicy> policy_;

  DISALLOW_COPY_AND_ASSIGN(MediaStateManagerImpl);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_NEVA_MEDIA_STATE_MANAGER_IMPL_H
