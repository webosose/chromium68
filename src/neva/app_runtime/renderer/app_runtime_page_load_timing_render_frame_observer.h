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

#ifndef NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_
#define NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_

#include "base/time/time.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_frame_observer.h"

namespace content {
class RenderView;
}

namespace app_runtime {

class AppRuntimePageLoadTimingRenderFrameObserver
    : public content::RenderFrameObserver {
 public:
  explicit AppRuntimePageLoadTimingRenderFrameObserver(
      content::RenderFrame* render_frame);
  ~AppRuntimePageLoadTimingRenderFrameObserver() override;

  // RenderFrameObserver implementation
  void DidChangePerformanceTiming() override;
  void DidNonFirstMeaningPaintAfterLoad() override;
  void DidResetStateToMarkNextPaintForContainer() override;

  void OnDestruct() override;

 private:
  bool HasNoRenderFrame() const;
  bool PageLoadTimingIsFirstMeaningful();
  base::Optional<base::TimeDelta> first_meaningful_paint_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_PAGE_LOAD_TIMING_RENDER_FRAME_OBSERVER_H_
