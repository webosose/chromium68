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

#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"

#include "base/memory/memory_pressure_listener.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_view.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"
#include "third_party/blink/public/platform/web_size.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_frame_widget.h"
#include "third_party/blink/public/web/web_view.h"

namespace app_runtime {

AppRuntimeRenderFrameObserver::AppRuntimeRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      dom_suspended_(false),
      binding_(this) {
  render_frame->GetAssociatedInterfaceRegistry()->AddInterface(base::Bind(
      &AppRuntimeRenderFrameObserver::BindRequest, base::Unretained(this)));
}

AppRuntimeRenderFrameObserver::~AppRuntimeRenderFrameObserver() = default;

void AppRuntimeRenderFrameObserver::BindRequest(
    mojom::AppRuntimeClientAssociatedRequest request) {
  binding_.Bind(std::move(request));
}

void AppRuntimeRenderFrameObserver::OnDestruct() {
}

bool AppRuntimeRenderFrameObserver::OnMessageReceived(
    const IPC::Message& message) {
  return false;
}

void AppRuntimeRenderFrameObserver::SetBackgroundColor(int32_t r,
                                                       int32_t g,
                                                       int32_t b,
                                                       int32_t a) {
  SkColor color = SkColorSetARGB(a, r, g, b);
  render_frame()->GetRenderView()->GetWebFrameWidget()
      ->SetBaseBackgroundColor(color);
}

void AppRuntimeRenderFrameObserver::SetViewportSize(int32_t width, int32_t height) {
  render_frame()->GetRenderView()->GetWebView()->MainFrame()->SetViewportSize(
      blink::WebSize(width, height));
}

void AppRuntimeRenderFrameObserver::SuspendDOM() {
  if (dom_suspended_)
    return;
  dom_suspended_ = true;

  render_frame()->GetRenderView()->GetWebView()->WillEnterModalLoop();
}

void AppRuntimeRenderFrameObserver::ResumeDOM() {
  if (!dom_suspended_)
    return;
  dom_suspended_ = false;

  render_frame()->GetRenderView()->GetWebView()->DidExitModalLoop();
}

void AppRuntimeRenderFrameObserver::ResetStateToMarkNextPaintForContainer() {
  render_frame()->ResetStateToMarkNextPaintForContainer();
}

void AppRuntimeRenderFrameObserver::SetVisibilityState(
    blink::mojom::PageVisibilityState visibility_state) {
  render_frame()->GetRenderView()->GetWebView()->SetVisibilityState(
      visibility_state,
      visibility_state == blink::mojom::PageVisibilityState::kLaunching);
}

void AppRuntimeRenderFrameObserver::DidClearWindowObject() {
  mojom::AppRuntimeHostAssociatedPtr interface;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(
      &interface);
  interface->DidClearWindowObject();
}

void AppRuntimeRenderFrameObserver::OnNotifyMemoryPressure(int level) {
  switch (level) {
    case 1:
      base::MemoryPressureListener::NotifyMemoryPressure(
          base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE);
      break;
    case 2:
      base::MemoryPressureListener::NotifyMemoryPressure(
          base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL);
      break;
    default:
      break;
  }
}

}  // namespace app_runtime
