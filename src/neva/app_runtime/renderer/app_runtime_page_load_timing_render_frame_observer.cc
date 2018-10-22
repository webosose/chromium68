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

#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"

#include "base/time/time.h"
#include "neva/app_runtime/common/app_runtime.mojom.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/web/web_performance.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace app_runtime {

namespace {

base::TimeDelta ClampDelta(double event, double start) {
  if (event - start < 0)
    event = start;
  return base::Time::FromDoubleT(event) - base::Time::FromDoubleT(start);
}

}  // namespace

AppRuntimePageLoadTimingRenderFrameObserver::
    AppRuntimePageLoadTimingRenderFrameObserver(
        content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {}

AppRuntimePageLoadTimingRenderFrameObserver::
    ~AppRuntimePageLoadTimingRenderFrameObserver() {}

void AppRuntimePageLoadTimingRenderFrameObserver::DidChangePerformanceTiming() {
  // Check frame exists
  if (HasNoRenderFrame())
    return;

  if (PageLoadTimingIsFirstMeaningful()) {
    const blink::WebPerformance& perf =
        render_frame()->GetWebFrame()->Performance();
    mojom::AppRuntimeHostAssociatedPtr interface;
    render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
    if (interface)
      interface->DidFirstMeaningfulPaint(perf.FirstMeaningfulPaint());
  }
}

void AppRuntimePageLoadTimingRenderFrameObserver::
    DidNonFirstMeaningPaintAfterLoad() {
  // Check frame exists
  if (HasNoRenderFrame())
    return;

  mojom::AppRuntimeHostAssociatedPtr interface;
  render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
  if (interface)
    interface->DidNonFirstMeaningfulPaint();
}

void AppRuntimePageLoadTimingRenderFrameObserver::
    DidResetStateToMarkNextPaintForContainer() {
  first_meaningful_paint_ = base::nullopt;
}

void AppRuntimePageLoadTimingRenderFrameObserver::OnDestruct() {
  delete this;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::HasNoRenderFrame() const {
  bool no_frame = !render_frame() || !render_frame()->GetWebFrame();
  DCHECK(!no_frame);
  return no_frame;
}

bool AppRuntimePageLoadTimingRenderFrameObserver::
    PageLoadTimingIsFirstMeaningful() {
  if (first_meaningful_paint_)
    return false;

  const blink::WebPerformance& perf =
      render_frame()->GetWebFrame()->Performance();

  if (perf.FirstMeaningfulPaint() > 0.0) {
    first_meaningful_paint_ =
        ClampDelta(perf.FirstMeaningfulPaint(), perf.NavigationStart());
    return true;
  }
  return false;
}

}  // namespace app_runtime
