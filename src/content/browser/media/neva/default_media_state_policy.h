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

#ifndef CONTENT_BROWSER_MEDIA_NEVA_DEFAULT_MEDIA_STATE_POLICY_H
#define CONTENT_BROWSER_MEDIA_NEVA_DEFAULT_MEDIA_STATE_POLICY_H

#include <list>

#include "content/browser/media/neva/media_state_policy.h"

namespace content {

// This class implements a basic policy for managing media states.
class CONTENT_EXPORT DefaultMediaStatePolicy : public MediaStatePolicy {
 public:
  DefaultMediaStatePolicy(Client* client);
  ~DefaultMediaStatePolicy() override {}

  void Initialize() override;
  void OnMediaActivated(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaActivationRequested(RenderFrameHost* render_frame_host,
                                  int player_id) override;
  void OnMediaCreated(RenderFrameHost* render_frame_host,
                      int player_id) override;
  void OnMediaDestroyed(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaResumeRequested(WebContents* web_contents) override;
  void OnMediaResumeRequested(RenderFrameHost* render_frame_host) override;
  void OnMediaSuspended(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaSuspendRequested(RenderFrameHost* render_frame_host) override;
  void OnMediaSuspendRequested(WebContents* web_contents) override;
  void OnRenderFrameDeleted(RenderFrameHost* render_frame_host) override;

 private:
  using MediaPlayerId = std::pair<RenderFrameHost*, int>;
  using MediaPlayerList = std::map<RenderFrameHost*, std::set<int>>;

  MediaPlayerList player_list_;

  DISALLOW_COPY_AND_ASSIGN(DefaultMediaStatePolicy);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_NEVA_DEFAULT_MEDIA_STATE_POLICY_H
