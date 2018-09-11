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

#include "neva/app_runtime/browser/app_runtime_http_user_agent_settings.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "net/http/http_util.h"
#include "ui/base/l10n/l10n_util.h"

namespace app_runtime {

AppRuntimeHttpUserAgentSettings::AppRuntimeHttpUserAgentSettings() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

AppRuntimeHttpUserAgentSettings::~AppRuntimeHttpUserAgentSettings() {
  DCHECK(content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
}

std::string AppRuntimeHttpUserAgentSettings::GetAcceptLanguage() const {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  return std::string();
}

std::string AppRuntimeHttpUserAgentSettings::GetUserAgent() const {
  return std::string();
}

}  // namespace app_runtime
