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

#include "content/renderer/neva/media_suppressor_impl.h"

#include "content/renderer/render_frame_impl.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace content {
namespace neva {

MediaSuppressorImpl::MediaSuppressorImpl(
    RenderFrameImpl* render_frame_impl)
    : media_suppressor_binding_(this),
      render_frame_impl_(render_frame_impl) {}

MediaSuppressorImpl::~MediaSuppressorImpl() {}

void MediaSuppressorImpl::Bind(
        mojom::MediaSuppressorAssociatedRequest request) {
  media_suppressor_binding_.Bind(std::move(request));
}

void MediaSuppressorImpl::SuspendMedia() {
    blink::WebLocalFrame* web_frame = render_frame_impl_->GetWebFrame();
    if (web_frame) {
      web_frame->SetSuppressMediaPlay(true);
    }
    for (auto& observer : render_frame_impl_->observers_) {
      observer.OnSuppressedMediaPlay(true);
    }
}

void MediaSuppressorImpl::ResumeMedia() {
    blink::WebLocalFrame* web_frame = render_frame_impl_->GetWebFrame();
    if (web_frame) {
      web_frame->SetSuppressMediaPlay(false);
    }
    for (auto& observer : render_frame_impl_->observers_) {
      observer.OnSuppressedMediaPlay(false);
    }
}

} //namespace neva
} //namesapce content
