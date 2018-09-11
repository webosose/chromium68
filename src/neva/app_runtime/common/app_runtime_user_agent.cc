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

#include "neva/app_runtime/common/app_runtime_user_agent.h"

#include "base/command_line.h"
#include "components/version_info/version_info.h"
#include "content/public/common/user_agent.h"
#include "net/http/http_util.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"

namespace app_runtime {

namespace {

std::string GetProduct() {
  return version_info::GetProductNameAndVersionForUserAgent();
}

}  // namespace

std::string GetUserAgent() {
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kUserAgent)) {
    std::string ua = command_line->GetSwitchValueASCII(kUserAgent);
    if (net::HttpUtil::IsValidHeaderValue(ua))
      return ua;
    LOG(WARNING) << "Ignored invalid value for flag --" << kUserAgent;
  }

  std::string product = GetProduct();
  return content::BuildUserAgentFromProduct(product);
}

}  // namespace app_runtime
