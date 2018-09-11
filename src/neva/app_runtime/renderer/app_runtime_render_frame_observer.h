// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_RENDER_FRAME_OBSERVER_H_
#define NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_RENDER_FRAME_OBSERVER_H_

#include "content/public/renderer/render_frame_observer.h"
#include "neva/app_runtime/common/app_runtime.mojom.h"
#include "mojo/public/cpp/bindings/associated_binding.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"

namespace app_runtime {

class AppRuntimeRenderFrameObserver : public content::RenderFrameObserver,
                                      public mojom::AppRuntimeClient {
 public:
  AppRuntimeRenderFrameObserver(content::RenderFrame* render_frame);
  ~AppRuntimeRenderFrameObserver() override;

  // RenderFrameObserver
  void OnDestruct() override;
  void DidClearWindowObject() override;

  // IPC message handlers
  void OnNotifyMemoryPressure(int level);

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override;

  // mojom::AppRuntimeClient implementation.
  void SetBackgroundColor(int32_t r, int32_t g, int32_t b, int32_t a) override;
  void SetViewportSize(int32_t width, int32_t height) override;
  void SuspendDOM() override;
  void ResumeDOM() override;
  void ResetStateToMarkNextPaintForContainer() override;
  void SetVisibilityState(blink::mojom::PageVisibilityState visibility_state) override;

  void BindRequest(mojom::AppRuntimeClientAssociatedRequest request);

 private:
  // Whether DOM activity is suspended or not.
  bool dom_suspended_;

  mojo::AssociatedBinding<mojom::AppRuntimeClient> binding_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_RENDER_FRAME_OBSERVER_H_
