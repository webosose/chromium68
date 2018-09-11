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

#include "webos/app/webos_content_main_delegate.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"
#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"
#include "webos/common/webos_resource_delegate.h"

using base::CommandLine;

namespace webos {

WebOSContentMainDelegate::WebOSContentMainDelegate() {}

bool WebOSContentMainDelegate::BasicStartupComplete(int* exit_code) {
  base::CommandLine* parsedCommandLine = base::CommandLine::ForCurrentProcess();
  // TODO(pikulik): should be revised
  // parsedCommandLine->AppendSwitch(switches::kNoSandbox);
  content::SetContentClient(&content_client_);

  std::string process_type =
        parsedCommandLine->GetSwitchValueASCII(switches::kProcessType);
  if (process_type.empty()) {
    startup_callback_.Run();
  }
  return false;
}

}  // namespace webos
