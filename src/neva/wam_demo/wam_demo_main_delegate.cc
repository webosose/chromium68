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

#include "neva/wam_demo/wam_demo_main_delegate.h"

#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "content/public/common/content_switches.h"
#include "neva/wam_demo/wam_demo_service.h"

namespace wam_demo {

WamDemoMainDelegate::WamDemoMainDelegate(
    const content::MainFunctionParams& parameters)
  : parameters_(parameters) {}

WamDemoMainDelegate::~WamDemoMainDelegate() {}

void WamDemoMainDelegate::PreMainMessageLoopRun() {
  service_ = std::make_unique<WamDemoService>(parameters_);
  if (parameters_.command_line.GetArgs().empty())
    return;

  const std::string appname("cmdline.app");
  const std::string appurl = parameters_.command_line.GetArgs()[0];
  const bool fullscreen = parameters_.command_line.HasSwitch("full-screen");
  const bool frameless = false;
  service_->Launch(appname, appurl, fullscreen, frameless);
}

}  // namespace wam_demo
