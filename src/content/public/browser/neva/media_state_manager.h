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

#ifndef CONTENT_PUBLIC_BROWSER_NEVA_MEDIA_STATE_MANAGER_H
#define CONTENT_PUBLIC_BROWSER_NEVA_MEDIA_STATE_MANAGER_H

#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/web_contents.h"

namespace content {

// MediaStateManager manages suspend state of entire media players.
// This gives useful functionalities on web based application or limited
// environment such as embedded device. For example, we can suspend all
// media players in a WebContents. Also by suspending a media, we can get a
// chance to unload media resource. And reload media resource when it is
// resumed.
// But note that actual unloading media resource is responsibility of each
// media player.
class CONTENT_EXPORT MediaStateManager {
 public:
  static MediaStateManager* GetInstance();

  // Called when a media player activated.
  virtual void OnMediaActivated(RenderFrameHost* render_frame_host,
                                int player_id) = 0;

  // Called when a media player needs to load, play, resume.
  virtual void OnMediaActivationRequested(RenderFrameHost* render_frame_host,
                                          int player_id) = 0;

  // Called when a media player is created. Usually this API is called by
  // MediaWebContentsObserver.
  virtual void OnMediaCreated(RenderFrameHost* render_frame_host,
                              int player_id) = 0;

  // Called when a media player is destroyed. Usually this API is called by
  // MediaWebContentsObserver.
  virtual void OnMediaDestroyed(RenderFrameHost* render_frame_host,
                                int player_id) = 0;

  // Called when a media player is suspended.
  virtual void OnMediaSuspended(RenderFrameHost* render_frame_host,
                                int player_id) = 0;

  // Usually managing WebContents and RenderFrameHost is done internally through
  // MediaWebContentsObserver. So embedder don't need to call this method.
  virtual void OnRenderFrameDeleted(RenderFrameHost* render_frame_host) = 0;

  // Usually managing WebContents and RenderFrameHost is done internally through
  // MediaWebContentsObserver. So embedder don't need to call this method.
  virtual void OnWebContentsDestroyed(WebContents* web_contents) = 0;

  // Resume all media players in a WebContents.
  // Suppressed state will be cleared. But actually resumed media players maybe
  // restricted by policy.
  virtual void ResumeAllMedia(WebContents* web_contents) = 0;

  // Resume all media players in a RenderFrameHost.
  // Suppressed state will be cleared. But actually resumed media players maybe
  // restricted by policy.
  virtual void ResumeAllMedia(RenderFrameHost* render_frame_host) = 0;

  // Suspend all media players in a WebContents.
  // All media players in this WebContents will be suppressed. This means that
  // new created media player will be suspended.
  virtual void SuspendAllMedia(WebContents* web_contents) = 0;

  // Suspend all media players in a RenderFrameHost.
  // All media players in this RenderFrameHost will be suppressed. This means
  // that new created media player will be suspended.
  virtual void SuspendAllMedia(RenderFrameHost* render_frame_host) = 0;

 protected:
  MediaStateManager() {}
  virtual ~MediaStateManager() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_NEVA_MEDIA_STATE_MANAGER_H
