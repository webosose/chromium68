// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_
#define THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_

#include "third_party/blink/public/platform/web_media_player.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_string.h"

namespace blink {
namespace neva {

class WebMediaPlayerClient {
 public:
  virtual WebString ContentMIMEType() const = 0;
  virtual WebString ContentTypeCodecs() const = 0;
  virtual WebString ContentCustomOption() const = 0;
  virtual WebString ContentMediaOption() const = 0;
  virtual WebString Referrer() const = 0;
  virtual WebString UserAgent() const = 0;
  virtual WebString Cookies() const = 0;
  virtual bool IsVideo() const = 0;
  virtual bool IsSuppressedMediaPlay() const = 0;
  virtual blink::WebMediaPlayer::LoadType LoadType() const = 0;
  virtual WebRect ScreenRect() = 0;
  virtual WebMediaPlayer::RenderMode RenderMode() const = 0;
  virtual WebRect WebWidgetViewRect() = 0;
  virtual void OnAudioFocusChanged() = 0;
  virtual void SendCustomMessage(const blink::WebMediaPlayer::MediaEventType,
                                 const WebString&) = 0;

 protected:
  ~WebMediaPlayerClient() = default;
};

}  // namespace neva
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_PUBLIC_PLATFORM_NEVA_WEB_MEDIA_PLAYER_CLIENT_H_
