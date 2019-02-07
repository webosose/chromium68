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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_

#include <stdio.h>
#include <type_traits>

#include "third_party/blink/renderer/core/dom/events/custom_event.h"
#include "third_party/blink/renderer/core/frame/local_frame.h"
#include "third_party/blink/renderer/core/frame/settings.h"
#include "third_party/blink/renderer/core/frame/web_frame_widget_base.h"
#include "third_party/blink/renderer/core/frame/web_local_frame_impl.h"
#include "third_party/blink/renderer/core/html/time_ranges.h"
#include "third_party/blink/renderer/core/loader/cookie_jar.h"
#include "third_party/blink/renderer/core/page/page.h"
#include "third_party/blink/renderer/platform/network/mime/content_type.h"
#include "third_party/blink/renderer/platform/weborigin/security_policy.h"
#include "third_party/blink/public/platform/web_media_player_client.h"
#include "third_party/blink/public/platform/web_rect.h"
#include "third_party/blink/public/platform/web_screen_info.h"
#include "third_party/blink/public/web/web_widget_client.h"

namespace blink {
namespace neva {

template <typename original_t>
class HTMLMediaElement {
 public:
  HTMLMediaElement();

  double getStartDate() const;
  const String cameraId() const;
  const String mediaId() const;
  bool IsPlayed() const;

  const IntRect VideoRectInScreen() const;
  const IntRect WidgetViewRect() const;

  // Neva audio focus extensions
  bool webosMediaFocus() const;
  void setWebosMediaFocus(bool focus);

 protected:
  void ScheduleEvent(const AtomicString& event_name, const String& detail);
  void ParseContentType(const ContentType& contentType);
  void ApplyVisibility(const bool);

  // platform(ex webos) media framework updates the time at 200ms intervals.
  // Set the follwing in consideration of the overlap interval.
  void SetMaxTimeupdateEventFrequency();

  bool cached_audio_focus_ : 1;

  // Holds the "timeline offset" as described in the HTML5 spec. Represents the
  // number of seconds since January 1, 1970 or NaN if no offset is in effect.
  double m_timelineOffset;

  // The spec says to fire periodic timeupdate events (those sent while playing)
  // every "15 to 250ms". we choose the slowest frequency
  TimeDelta kMaxTimeupdateEventFrequency;

  String m_contentMIMEType;
  String m_contentTypeCodecs;
  String m_contentCustomOption;
  String m_contentMediaOption;
  blink::WebMediaPlayer::RenderMode m_renderMode;
};

template <typename original_t>
HTMLMediaElement<original_t>::HTMLMediaElement()
    : cached_audio_focus_(false),
      m_timelineOffset(std::numeric_limits<double>::quiet_NaN()),
      m_renderMode(blink::WebMediaPlayer::RenderModeHole) {}

template <typename original_t>
double HTMLMediaElement<original_t>::getStartDate() const {
  return m_timelineOffset;
}

template <typename original_t>
const IntRect HTMLMediaElement<original_t>::VideoRectInScreen() const {
  const original_t* self(static_cast<const original_t*>(this));

  if (!self->GetLayoutObject())
    return IntRect();

  // FIXME: this should probably respect transforms
  return self->GetDocument().View()->ContentsToScreen(
      self->GetLayoutObject()->AbsoluteBoundingBoxRectIgnoringTransforms());
}

template <typename original_t>
const IntRect HTMLMediaElement<original_t>::WidgetViewRect() const {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();

  if (!frame)
    return IntRect();

  WebFrameWidgetBase* frame_widget =
      WebLocalFrameImpl::FromFrame(frame)->LocalRootFrameWidget();

  if (!frame_widget)
    return IntRect();

  WebRect rect = frame_widget->Client()->ViewRect();
  return IntRect(rect.x, rect.y, rect.width, rect.height);
}

template <typename original_t>
const String HTMLMediaElement<original_t>::cameraId() const {
  const original_t* self(static_cast<const original_t*>(this));

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->CameraId();
  return String();
}

template <typename original_t>
const String HTMLMediaElement<original_t>::mediaId() const {
  const original_t* self(static_cast<const original_t*>(this));
  DCHECK(RuntimeEnabledFeatures::UMSExtensionEnabled());

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->MediaId();
  return String();
}

template <typename original_t>
bool HTMLMediaElement<original_t>::IsPlayed() const {
  const original_t* self(static_cast<const original_t*>(this));
  bool result = self->playing_;

  if (self->played_time_ranges_ &&
      self->played_time_ranges_->length() != 0)
    result = true;
  return result;
}

template <typename original_t>
bool HTMLMediaElement<original_t>::webosMediaFocus() const {
  DCHECK(RuntimeEnabledFeatures::AudioFocusExtensionEnabled());

  const original_t* self(static_cast<const original_t*>(this));

  if (self->GetWebMediaPlayer())
    return self->GetWebMediaPlayer()->HasAudioFocus();
  return cached_audio_focus_;
}

template <typename original_t>
void HTMLMediaElement<original_t>::setWebosMediaFocus(bool focus) {
  DCHECK(RuntimeEnabledFeatures::AudioFocusExtensionEnabled());

  original_t* self(static_cast<original_t*>(this));

  if (self->GetWebMediaPlayer())
    self->GetWebMediaPlayer()->SetAudioFocus(focus);
}

template <typename original_t>
void HTMLMediaElement<original_t>::ScheduleEvent(const AtomicString& event_name,
                                                 const String& detail) {
  original_t* self(static_cast<original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (!frame) {
    LOG(ERROR) << "Document has no frame";
    return;
  }

  ScriptState* script_state = ToScriptStateForMainWorld(frame);
  if (!script_state) {
    LOG(ERROR) << "ScriptState is null";
    return;
  }

  blink::CustomEvent* event = CustomEvent::Create();
  ScriptState::Scope script_scope(script_state);
  event->initCustomEvent(script_state, event_name, false, true,
                         ScriptValue::From(script_state, detail));

  self->async_event_queue_->EnqueueEvent(FROM_HERE, event);
}

template <typename original_t>
void HTMLMediaElement<original_t>::ParseContentType(
    const ContentType& contentType) {
  DEFINE_STATIC_LOCAL(const String, codecs, ("codecs"));
  DEFINE_STATIC_LOCAL(const String, mediaOption, ("mediaOption"));
  DEFINE_STATIC_LOCAL(const String, cameraOption, ("cameraOption"));
  DEFINE_STATIC_LOCAL(const String, dvrOption, ("dvrOption"));

  m_contentMIMEType = contentType.GetType().LowerASCII();
  m_contentTypeCodecs = contentType.Parameter(codecs);

  m_contentMediaOption =
      DecodeURLEscapeSequences(contentType.Parameter(mediaOption));
  if (!m_contentMediaOption.IsEmpty())
    VLOG(1) << "mediaOption=[" << m_contentMediaOption.Utf8().data() << "]";
  if (m_contentMIMEType == "service/webos-camera")
    m_contentCustomOption =
        DecodeURLEscapeSequences(contentType.Parameter(cameraOption));
  else if (m_contentMIMEType == "service/webos-dvr")
    m_contentCustomOption =
        DecodeURLEscapeSequences(contentType.Parameter(dvrOption));
}

template <typename original_t>
void HTMLMediaElement<original_t>::ApplyVisibility(const bool visibility) {
  original_t* self(static_cast<original_t*>(this));

  auto player = self->GetWebMediaPlayer();

  // Do not use player->HasVisibility() in here.
  if (player)
    player->SetVisibility(visibility);
}

template <typename original_t>
void HTMLMediaElement<original_t>::SetMaxTimeupdateEventFrequency() {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (frame) {
    Settings* settings = frame->GetSettings();
    if (settings) {
      kMaxTimeupdateEventFrequency = TimeDelta::FromMilliseconds(
          settings->GetMaxTimeupdateEventFrequency());
    }
  }
}

template <typename original_t>
class HTMLMediaElementExtendingWebMediaPlayerClient
    : public blink::WebMediaPlayerClient {
 public:
  void SendCustomMessage(const blink::WebMediaPlayer::MediaEventType,
                         const blink::WebString&) override;
  WebString ContentMIMEType() const override;
  WebString ContentTypeCodecs() const override;
  WebString ContentCustomOption() const override;
  WebString ContentMediaOption() const override;
  WebString Referrer() const override;
  WebString UserAgent() const override;
  WebString Cookies() const override;
  bool IsSuppressedMediaPlay() const override;
  blink::WebMediaPlayer::LoadType LoadType() const override;
  bool IsVideo() const override;
  WebRect ScreenRect() override;
  WebMediaPlayer::RenderMode RenderMode() const override;
  WebRect WebWidgetViewRect() override;

  // Neva audio focus extension
  void OnAudioFocusChanged() override;
};

template <typename original_t>
WebRect
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::WebWidgetViewRect() {
  const original_t* self(static_cast<original_t*>(this));
  return self->WidgetViewRect();
}

template <typename original_t>
void HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::SendCustomMessage(const blink::WebMediaPlayer::MediaEventType
                                       media_event_type,
                                   const blink::WebString& detail) {
  original_t* self(static_cast<original_t*>(this));
  switch (media_event_type) {
    case blink::WebMediaPlayer::kMediaEventUpdateUMSMediaInfo:
      if (RuntimeEnabledFeatures::UMSExtensionEnabled())
        self->ScheduleEvent(EventTypeNames::umsmediainfo, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventPipelineStarted:
      self->ScheduleEvent(EventTypeNames::pipelinestarted, detail);
      break;
    case blink::WebMediaPlayer::kMediaEventUpdateCameraState:
      self->ScheduleEvent(EventTypeNames::updatecamerastate, detail);
      break;
    default:
      break;
  }
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentMIMEType() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentMIMEType;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentTypeCodecs() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentTypeCodecs;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentCustomOption() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentCustomOption;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::ContentMediaOption() const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->m_contentMediaOption;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::Referrer()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  return SecurityPolicy::GenerateReferrer(
             self->GetDocument().GetReferrerPolicy(), self->currentSrc(),
             self->GetDocument().OutgoingReferrer())
      .referrer;
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::UserAgent()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  LocalFrame* frame = self->GetDocument().GetFrame();
  if (!frame)
    return String();

  return frame->Loader().UserAgent();
}

template <typename original_t>
WebString HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::Cookies()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  return ::blink::Cookies(self->ownerDocument(), self->currentSrc());
}

template <typename original_t>
bool HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::IsVideo()
    const {
  const original_t* self(static_cast<const original_t*>(this));

  return self->IsHTMLVideoElement();
}

template <typename original_t>
bool HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::IsSuppressedMediaPlay() const {
  const original_t* self(static_cast<const original_t*>(this));
  LocalFrame* frame = self->GetDocument().GetFrame();
  if (frame) {
    WebLocalFrameImpl* weblocalframeimpl = WebLocalFrameImpl::FromFrame(frame);
    if (weblocalframeimpl)
        return weblocalframeimpl->IsSuppressedMediaPlay();
  }

  return false;
}

template <typename original_t>
blink::WebMediaPlayer::LoadType
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::LoadType() const {
  const original_t* self(static_cast<const original_t*>(this));

  if (self->media_source_)
    return blink::WebMediaPlayer::kLoadTypeMediaSource;

  if (self->GetSrcObject() &&
      (!self->current_src_.IsNull() &&
       self->IsMediaStreamURL(self->current_src_.GetString())))
    return blink::WebMediaPlayer::kLoadTypeMediaStream;

  return blink::WebMediaPlayer::kLoadTypeURL;
}

template <typename original_t>
WebRect
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::ScreenRect() {
  original_t* self(static_cast<original_t*>(this));
  LocalFrame* frame = self->GetDocument().GetFrame();

  if (!frame)
    return WebRect();

  WebFrameWidgetBase* frame_widget =
      WebLocalFrameImpl::FromFrame(frame)->LocalRootFrameWidget();

  if (!frame_widget)
    return WebRect();

  WebScreenInfo screen_info = frame_widget->Client()->GetScreenInfo();

  // We may consider screen_info.available_rect if screen_info.rect gives
  // incorrect value.
  return screen_info.rect;
}

template <typename original_t>
WebMediaPlayer::RenderMode
HTMLMediaElementExtendingWebMediaPlayerClient<original_t>::RenderMode() const {
  const original_t* self(static_cast<const original_t*>(this));
  if (self->IsHTMLVideoElement())
    return self->m_renderMode;
  return blink::WebMediaPlayer::RenderModeDefault;
}

template <typename original_t>
void HTMLMediaElementExtendingWebMediaPlayerClient<
    original_t>::OnAudioFocusChanged() {
  original_t* self(static_cast<original_t*>(this));

  if (!RuntimeEnabledFeatures::AudioFocusExtensionEnabled())
    return;

  if (self->GetWebMediaPlayer())
    self->cached_audio_focus_ = self->GetWebMediaPlayer()->HasAudioFocus();

  self->ScheduleEvent(EventTypeNames::webosmediafocuschange);
}

}  // namespace neva
}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_HTML_MEDIA_NEVA_HTML_MEDIA_ELEMENT_H_
