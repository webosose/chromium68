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

#include "content/browser/media/neva/limited_media_activation_count_policy.h"

#include <algorithm>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/time/time.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_switches.h"

namespace content {

static const int kDefaultMaxActivatedMediaNumber = 1;
static const int kTimeoutForNoResponseMediaMS = 20000;  // 20 seconds

LimitedMediaActivationCountPolicy::LimitedMediaActivationCountPolicy(
    Client* client)
    : MediaStatePolicy(client),
      max_capacity_(kDefaultMaxActivatedMediaNumber),
      token_(0) {}

void LimitedMediaActivationCountPolicy::Initialize() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  CHECK(command_line.HasSwitch(switches::kMaxActivatedMediaPlayers));
  max_capacity_ =
      atoi(command_line.GetSwitchValueASCII(switches::kMaxActivatedMediaPlayers)
               .c_str());
  CHECK(max_capacity_ > 0);
}

void LimitedMediaActivationCountPolicy::OnMediaActivated(
    RenderFrameHost* render_frame_host,
    int player_id) {
  MediaPlayerId id(render_frame_host, player_id);

  if (Contains(unlimited_player_list_, id))
    return;

  // There are some possibilities in here:
  // 1) This media player was already suspended.
  // 2) This media player sent response message multiple times.
  // We can detect both cases by checking waiting_response_list_.
  if (!Contains(waiting_response_list_, id))
    return;

  if (!Contains(activated_list_, id))
    activated_list_.push_back(id);

  waiting_response_list_.remove(id);
  ProcessPendingList();
}

void LimitedMediaActivationCountPolicy::OnMediaActivationRequested(
    RenderFrameHost* render_frame_host,
    int player_id) {
  if (!client_)
    return;

  MediaPlayerId id(render_frame_host, player_id);

  // Just permit immediately if the media player is in unlimited_player_list_.
  if (Contains(unlimited_player_list_, id)) {
    client_->PermitMediaActivation(id.first, id.second);
    return;
  }

  // We don't do anything if the media player is not in deactivated list.
  if (!Contains(deactivated_list_, id))
    return;

  if (!Contains(pending_request_list_, id))
    pending_request_list_.push_back(id);

  deactivated_list_.remove(id);
  ProcessPendingList();
}

void LimitedMediaActivationCountPolicy::OnMediaCreated(
    RenderFrameHost* render_frame_host,
    int player_id,
    bool will_use_media_resource) {
  MediaPlayerId id(render_frame_host, player_id);

  RemoveIdInAllList(id);

  if (will_use_media_resource) {
    deactivated_list_.push_back(id);
  } else {
    unlimited_player_list_.push_back(id);
  }

  ids_[render_frame_host][player_id] = 0;
}

void LimitedMediaActivationCountPolicy::OnMediaDestroyed(
    RenderFrameHost* render_frame_host,
    int player_id) {
  MediaPlayerId id(render_frame_host, player_id);

  RemoveIdInAllList(id);
  ProcessPendingList();

  ids_[render_frame_host].erase(player_id);
}

void LimitedMediaActivationCountPolicy::OnMediaResumeRequested(
    WebContents* web_contents) {
  // We send resume messages to max_capacity_ of media players in WebContents.
  // For example,
  // wc1: rfh1, rfh2
  // wc2: rfh3
  // rfh1: mp1, mp2
  // rfh2: mp3
  // rfh3: mp4, mp5
  // deactivated_list_: [rfh1, mp1], [rfh1, mp2], [rfh2, mp3], [rfh3, mp4],
  //                    [rfh3, mp5]

  // max_capacity_: 2
  // Called OnMediaResumeRequested(wc1), then
  // Media players need to send resume message:
  // [rfh2, mp3], [rfh1, mp2]

  CheckList check_list;
  for (auto* rfh : web_contents->GetAllFrames())
    check_list.insert(rfh);
  ResumeMediaForCheckList(check_list);
}

void LimitedMediaActivationCountPolicy::OnMediaResumeRequested(
    RenderFrameHost* render_frame_host) {
  CheckList check_list;
  check_list.insert(render_frame_host);
  ResumeMediaForCheckList(check_list);
}

void LimitedMediaActivationCountPolicy::OnMediaSuspended(
    RenderFrameHost* render_frame_host,
    int player_id) {
  MediaPlayerId id(render_frame_host, player_id);

  if (Contains(deactivated_list_, id) || Contains(unlimited_player_list_, id))
    return;

  // We assume that a media player can be deactivated from any status(not only
  // pending deactivated list). So first do remove id in all list.
  RemoveIdInAllList(id);
  deactivated_list_.push_back(id);
  ProcessPendingList();
}

void LimitedMediaActivationCountPolicy::OnMediaSuspendRequested(
    WebContents* web_contents) {
  // We send suspend message to all non deactivated media players in
  // WebContents. For example,
  // wc1: rfh1, rfh2, rfh3
  // wc2: rfh4
  // rfh1: mp1, mp2
  // rfh2: mp3
  // rfh3: mp4
  // rfh4: mp5, mp6
  // activated_list_: [rfh1, mp1]
  // waiting_response_list_: [rfh2, mp3]
  // pending_request_list_: [rfh4, mp5]
  // pending_deactivated_list_: [rfh3, mp4]
  // deactivated_list_: [rfh1, mp2], [rfh4, mp6]

  // Called OnMediaSuspendRequested(wc1), then
  // Media players need to send suspend message:
  // [rfh1, mp1], [rfh2, mp3], [rfh1, mp2]
  // Note that we need to send suspend message to deactivated media players
  // because we need to suspend preloaded media players.

  CheckList check_list;
  for (auto* rfh : web_contents->GetAllFrames())
    check_list.insert(rfh);

  SuspendMediaIfExistIn(activated_list_, check_list);
  SuspendMediaIfExistIn(waiting_response_list_, check_list);
  SuspendMediaIfExistIn(pending_request_list_, check_list);
  SuspendMediaIfExistIn(deactivated_list_, check_list);
  SuspendMediaIfExistIn(unlimited_player_list_, check_list);
}

void LimitedMediaActivationCountPolicy::OnMediaSuspendRequested(
    RenderFrameHost* render_frame_host) {
  CheckList check_list;
  check_list.insert(render_frame_host);

  SuspendMediaIfExistIn(activated_list_, check_list);
  SuspendMediaIfExistIn(waiting_response_list_, check_list);
  SuspendMediaIfExistIn(pending_request_list_, check_list);
  SuspendMediaIfExistIn(deactivated_list_, check_list);
  SuspendMediaIfExistIn(unlimited_player_list_, check_list);
}

void LimitedMediaActivationCountPolicy::OnRenderFrameDeleted(
    RenderFrameHost* render_frame_host) {
  if (ids_.find(render_frame_host) == ids_.end())
    return;

  for (auto iter = ids_[render_frame_host].begin();
       iter != ids_[render_frame_host].end(); iter++)
    RemoveIdInAllList(MediaPlayerId(render_frame_host, iter->first));
  ids_.erase(render_frame_host);
  ProcessPendingList();
}

bool LimitedMediaActivationCountPolicy::Contains(const MediaPlayerList& list,
                                                 const MediaPlayerId& id) {
  return std::find(list.begin(), list.end(), id) != list.end();
}

int LimitedMediaActivationCountPolicy::GetAvailableNumber() {
  return max_capacity_ -
         (activated_list_.size() + waiting_response_list_.size() +
          pending_deactivated_list_.size());
}

int LimitedMediaActivationCountPolicy::GetToken(const MediaPlayerId& id) {
  return ids_[id.first][id.second];
}

void LimitedMediaActivationCountPolicy::GiveTokenToId(const MediaPlayerId& id) {
  ids_[id.first][id.second] = ++token_;
}

void LimitedMediaActivationCountPolicy::ProcessPendingList() {
  DCHECK(GetAvailableNumber() >= 0);

  if (!client_)
    return;

  // No pending, no action.
  if (pending_request_list_.size() == 0)
    return;

  // If we are fully using our resources, we try to deactivate activated
  // media players.
  if (GetAvailableNumber() == 0) {
    if (activated_list_.size() > 0) {
      MediaPlayerId id = *activated_list_.begin();
      pending_deactivated_list_.push_back(id);
      activated_list_.remove(id);
      client_->SuspendMedia(id.first, id.second);
    }

    return;
  }

  // If we have available resources, then promote some pending media players
  // into waiting response list.
  while (GetAvailableNumber() > 0) {
    bool processed = false;

    if (pending_request_list_.size() > 0) {
      MediaPlayerId id = *pending_request_list_.begin();
      waiting_response_list_.push_back(id);
      pending_request_list_.remove(id);
      client_->PermitMediaActivation(id.first, id.second);
      processed = true;

      // This is for exceptional case. If a media player never send response
      // for PermitMediaActivation(), we lost one media resource. And, if our
      // max_capacity_ is one, entire media players are in stuck until
      // corresponding render_frame_host is deleted. To prevent this worst case,
      // we use timer based approach: First we give a token to a media player
      // when the player enter waiting_response_list_. And trigger a timer.
      // We treat a media player as 'no responding' if following conditions are
      // meet(after timer is fired):
      // 1) The media player is in waiting_response_list_.
      // 2) The media player has same token.
      GiveTokenToId(id);
      BrowserThread::PostDelayedTask(
          BrowserThread::UI, FROM_HERE,
          base::BindOnce(
              &LimitedMediaActivationCountPolicy::SuspendNoRespondingMedia,
              base::Unretained(this), GetToken(id)),
          base::TimeDelta::FromMilliseconds(kTimeoutForNoResponseMediaMS));
    }

    if (!processed)
      break;
  }
}

void LimitedMediaActivationCountPolicy::RemoveIdInAllList(
    const MediaPlayerId& id) {
  activated_list_.remove(id);
  waiting_response_list_.remove(id);
  pending_request_list_.remove(id);
  deactivated_list_.remove(id);
  pending_deactivated_list_.remove(id);
  unlimited_player_list_.remove(id);
}

void LimitedMediaActivationCountPolicy::ResumeMediaForCheckList(
    CheckList& check_list) {
  if (!client_)
    return;

  // For unlimited players, just resume all media players in |check_list|.
  for (auto iter = unlimited_player_list_.begin();
       iter != unlimited_player_list_.end(); iter++) {
    if (check_list.find(iter->first) != check_list.end())
      client_->PermitMediaActivation(iter->first, iter->second);
  }

  // We assume that there are no any activated or pending media players in each
  // rfh. So we just find available media players in deactivated list.
  // We may think about resuming GetAvailableNumber() of media players.
  // But some scenarios are calling resume for a tab(WebContents) before suspend
  // , so we can't resume media players correctly if we use the value. So, in
  // here, just using max_capacity_ makes our policy more robust.

  for (int i = 0; i < max_capacity_; i++) {
    bool processed = false;

    // Find available media player from last element.
    for (auto iter = deactivated_list_.rbegin();
         iter != deactivated_list_.rend(); iter++) {
      if (check_list.find(iter->first) != check_list.end()) {
        OnMediaActivationRequested(iter->first, iter->second);
        processed = true;
        break;
      }
    }

    if (!processed)
      break;
  }
}

void LimitedMediaActivationCountPolicy::SuspendMediaIfExistIn(
    MediaPlayerList& list,
    CheckList& check_list) {
  if (!client_)
    return;

  for (auto iter = list.begin(); iter != list.end(); iter++) {
    if (check_list.find(iter->first) != check_list.end())
      client_->SuspendMedia(iter->first, iter->second);
  }
}

void LimitedMediaActivationCountPolicy::SuspendNoRespondingMedia(int token) {
  if (!client_)
    return;

  for (auto iter = waiting_response_list_.begin();
       iter != waiting_response_list_.end(); iter++) {
    if (ids_[iter->first][iter->second] == token) {
      client_->SuspendMedia(iter->first, iter->second);
    }
  }
}

}  // namespace content
