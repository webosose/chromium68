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

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_thread.h"
#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"

#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"
#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"

namespace app_runtime {

void AppRuntimeContentRendererClient::RenderThreadStarted() {

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableWatchdog)) {
    watchdog_.reset(new watchdog::Watchdog());

    std::string env_timeout =
        command_line.GetSwitchValueASCII(switches::kWatchdogRendererTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      base::StringToInt(env_timeout, &timeout);
      watchdog_->SetTimeout(timeout);
    }

    std::string env_period =
        command_line.GetSwitchValueASCII(switches::kWatchdogRendererPeriod);
    if (!env_period.empty()) {
      int period;
      base::StringToInt(env_period, &period);
      watchdog_->SetPeriod(period);
    }

    watchdog_->StartWatchdog();

    // Check it's currently running on RenderThread
    CHECK(content::RenderThread::Get());
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();
    task_runner->PostTask(
        FROM_HERE, base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                              base::Unretained(this)));
  }
}

void AppRuntimeContentRendererClient::ArmWatchdog() {
  watchdog_->Arm();
  if (!watchdog_->GetWatchingThreadTid())
    watchdog_->SetWatchingThreadTid((pid_t)syscall(SYS_gettid));

  // Check it's currently running on RenderThread
  CHECK(content::RenderThread::Get());
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();
  task_runner->PostDelayedTask(
      FROM_HERE,
      base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                 base::Unretained(this)),
      base::TimeDelta::FromSeconds(watchdog_->GetPeriod()));
}

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
