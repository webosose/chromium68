// Copyright (c) 2018 LG Electronics, Inc.
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

#include "neva/app_runtime/webapp_injection_manager.h"

#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "neva/injection/browser_control/browser_control_injection.h"
#include "neva/injection/palmsystem/palmsystem_injection.h"
#include "neva/injection/sample/sample_injection.h"
#include "neva/neva_chromium/content/common/injection_messages.h"

namespace app_runtime {

namespace {

std::set<std::string> allowed_injections = {
#if defined(OS_WEBOS)
  std::string(extensions_v8::PalmSystemInjectionExtension::kPalmSystemInjectionName),
#endif
#if defined(ENABLE_SAMPLE_WEBAPI)
  std::string(extensions_v8::SampleInjectionExtension::kInjectionName),
#endif
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  std::string(extensions_v8::BrowserControlInjectionExtension::kInjectionName),
#endif
};

}  // namespace

WebAppInjectionManager::~WebAppInjectionManager() {
}

void WebAppInjectionManager::RequestLoadExtension(const std::string& injection_name) {
  content::RenderViewHost* const rvh = web_contents_.GetRenderViewHost();
  if (rvh && (allowed_injections.count(injection_name) > 0)) {
    rvh->Send(new InjectionMsg_LoadExtension(rvh->GetRoutingID(),
              injection_name));
  }
}

void WebAppInjectionManager::RequestClearExtensions() {
  content::RenderViewHost* const rvh = web_contents_.GetRenderViewHost();
  if (rvh)
    rvh->Send(new InjectionMsg_ClearExtensions(rvh->GetRoutingID()));
}

}  // namespace app_runtime
