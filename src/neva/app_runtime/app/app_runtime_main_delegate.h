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

#ifndef NEVA_APP_RUNTIME_APP_APP_RUNTIME_MAIN_DELEGATE_H_
#define NEVA_APP_RUNTIME_APP_APP_RUNTIME_MAIN_DELEGATE_H_

#include "content/public/app/content_main_delegate.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_extra_parts.h"
#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"
#include "neva/app_runtime/common/app_runtime_content_client.h"

namespace net {
class NetworkDelegate;
}  // namespace net

namespace app_runtime {

AppRuntimeContentBrowserClient* GetAppRuntimeContentBrowserClient();
void SetBrowserContext(content::BrowserContext* p);
void SetNetworkDelegate(net::NetworkDelegate* p);

class AppRuntimeMainDelegate : public content::ContentMainDelegate
                             , public AppRuntimeBrowserMainExtraParts {
 public:
  AppRuntimeMainDelegate();
  ~AppRuntimeMainDelegate() override;

  // content::ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void PreSandboxStartup() override;
  void ProcessExiting(const std::string& process_type) override;

  content::ContentBrowserClient* CreateContentBrowserClient() override;
  content::ContentRendererClient* CreateContentRendererClient() override;

  // AppRuntimeBrowserMainExtraParts
  void PreMainMessageLoopRun() override;

 private:
  void InitializeResourceBundle();

  AppRuntimeContentClient content_client_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_APP_APP_RUNTIME_MAIN_DELEGATE_H_
