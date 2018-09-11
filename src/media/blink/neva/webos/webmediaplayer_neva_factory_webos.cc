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

#include "media/blink/neva/webmediaplayer_neva_factory.h"

#include "media/blink/neva/webmediaplayer_neva.h"
#include "media/blink/neva/webos/webmediaplayer_mse.h"

namespace media {

bool WebMediaPlayerNevaFactory::Playable(
    const blink::WebMediaPlayerClient* client) {
  int load_type = client->LoadType();
  switch (load_type) {
    case blink::WebMediaPlayer::kLoadTypeMediaSource:
      return true;
    case blink::WebMediaPlayer::kLoadTypeURL:
      if (WebMediaPlayerNeva::CanSupportMediaType(
              client->ContentMIMEType().Latin1()))
        return true;
  }

  return false;
}

blink::WebMediaPlayer* WebMediaPlayerNevaFactory::CreateWebMediaPlayerNeva(
    blink::WebLocalFrame* frame,
    blink::WebMediaPlayerClient* client,
    blink::WebMediaPlayerEncryptedMediaClient* encrypted_client,
    WebMediaPlayerDelegate* delegate,
    std::unique_ptr<RendererFactorySelector> renderer_factory_selector,
    UrlIndex* url_index,
    std::unique_ptr<VideoFrameCompositor> compositor,
    const StreamTextureFactoryCreateCB& stream_texture_factory_create_cb,
    std::unique_ptr<WebMediaPlayerParams> params) {
  if (client->LoadType() == blink::WebMediaPlayer::kLoadTypeMediaSource) {
    const blink::WebFloatPoint additional_contents_scale =
        params->additional_contents_scale();
    const blink::WebString& application_id = params->application_id();

    return new media::WebMediaPlayerMSE(
        frame, client, encrypted_client, delegate,
        std::move(renderer_factory_selector), url_index, std::move(compositor),
        stream_texture_factory_create_cb, std::move(params),
        additional_contents_scale, application_id);
  }

  if (client->LoadType() == blink::WebMediaPlayer::kLoadTypeURL)
    return media::WebMediaPlayerNeva::Create(frame, client, delegate,
                                             stream_texture_factory_create_cb,
                                             std::move(params));
  NOTREACHED();
  return nullptr;
}

}  // namespace media
