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

#ifndef CONTENT_BROWSER_MEDIA_NEVA_LIMITED_MEDIA_ACTIVATION_COUNT_POLICY_H
#define CONTENT_BROWSER_MEDIA_NEVA_LIMITED_MEDIA_ACTIVATION_COUNT_POLICY_H

#include <list>
#include <map>
#include <set>

#include "content/browser/media/neva/media_state_policy.h"

namespace content {

// This policy manages media players under the maximum number of activated
// media players. The value is provided via 'max-activated-media-players'
// command line switch.
// Also we handle some custome media player specially. The media player is
// allowed to activate immediately. And it will not introduce to suspend
// another media player, and vice versa. We distinguish this type of media
// player via |will_use_media_resource| in OnMediaCreated(). The media player
// will be suspended/resumed on corresponding render_frame_host is suspended
// /resumed.
class CONTENT_EXPORT LimitedMediaActivationCountPolicy
    : public MediaStatePolicy {
 public:
  LimitedMediaActivationCountPolicy(Client* client);
  ~LimitedMediaActivationCountPolicy() override {}

  void Initialize() override;
  void OnMediaActivated(RenderFrameHost* render_frame_host,
                        int player_id) override;
  void OnMediaActivationRequested(RenderFrameHost* render_frame_host,
                                  int player_id) override;
  void OnMediaCreated(RenderFrameHost* render_frame_host,
                      int player_id,
                      bool will_use_media_resource) override;
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
  using MediaPlayerList = std::list<MediaPlayerId>;
  using CheckList = std::set<RenderFrameHost*>;

  bool Contains(const MediaPlayerList& list, const MediaPlayerId& id);
  int GetAvailableNumber();
  int GetToken(const MediaPlayerId& id);
  void GiveTokenToId(const MediaPlayerId& id);
  void ProcessPendingList();
  void RemoveIdInAllList(const MediaPlayerId& id);
  void ResumeMediaForCheckList(CheckList& check_list);
  void SuspendMediaIfExistIn(MediaPlayerList& list, CheckList& check_list);
  void SuspendNoRespondingMedia(int token);

  MediaPlayerList activated_list_;
  MediaPlayerList waiting_response_list_;
  MediaPlayerList pending_request_list_;
  MediaPlayerList deactivated_list_;
  MediaPlayerList pending_deactivated_list_;

  MediaPlayerList unlimited_player_list_;

  // pair<player_id, token>
  std::map<RenderFrameHost*, std::map<int, int>> ids_;

  int max_capacity_;
  int token_;

  DISALLOW_COPY_AND_ASSIGN(LimitedMediaActivationCountPolicy);
};

}  // namespace content

#endif  // CONTENT_BROWSER_MEDIA_NEVA_LIMITED_MEDIA_ACTIVATION_COUNT_POLICY_H
