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

#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"

#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"
#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"

namespace app_runtime {

void AppRuntimeContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  // Only attach AppRuntimePageLoadTimingRenderFrameObserver to the main frame,
  // since we only want to observe page load timing for the main frame.
  if (render_frame->IsMainFrame()) {
    new AppRuntimeRenderFrameObserver(render_frame);
    new AppRuntimePageLoadTimingRenderFrameObserver(render_frame);
  }
}

}  // namespace app_runtime
