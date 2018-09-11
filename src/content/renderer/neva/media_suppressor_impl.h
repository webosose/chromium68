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

#ifndef CONTENT_RENDERER_NEVA_MEDIA_SUPPRESSOR_IMPL_H_
#define CONTENT_RENDERER_NEVA_MEDIA_SUPPRESSOR_IMPL_H_

#include "base/observer_list.h"
#include "content/common/media/neva/media_suppressor.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"

namespace content {
class RenderFrameImpl;
namespace neva {

class MediaSuppressorImpl
  : public content::mojom::MediaSuppressor {
 public:
  MediaSuppressorImpl(RenderFrameImpl* render_frame_impl);
  ~MediaSuppressorImpl() override;
  void SuspendMedia() override;
  void ResumeMedia() override;

  void Bind(content::mojom::MediaSuppressorAssociatedRequest);
 private:
  mojo::AssociatedBinding<mojom::MediaSuppressor>
      media_suppressor_binding_;
  RenderFrameImpl* render_frame_impl_;
};

} //namesapce neva
} //namespace content

#endif // CONTENT_RENDERER_NEVA_MEDIA_SUPPRESSOR_IMPL_H_

