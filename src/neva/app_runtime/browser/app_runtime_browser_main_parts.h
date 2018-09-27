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

#ifndef NEVA_APP_RUNTIME_APP_RUNTIME_BROWSER_MAIN_PARTS_H_
#define NEVA_APP_RUNTIME_APP_RUNTIME_BROWSER_MAIN_PARTS_H_

#include <memory>

#include "components/watchdog/watchdog.h"
#include "content/public/browser/browser_main_parts.h"
#include "content/public/browser/browser_thread.h"
#include "neva/app_runtime/browser/app_runtime_browser_context.h"

#if defined(ENABLE_PLUGINS)
#include "neva/app_runtime/browser/app_runtime_plugin_service_filter.h"
#endif

#include "neva/app_runtime/public/webapp_window_base.h"
#include "neva/app_runtime/public/webview_base.h"

namespace devtools_http_handler {
class DevToolsHttpHandler;
}  // namespace devtools_http_handler

namespace app_runtime {

class AppRuntimeBrowserMainExtraParts;
class AppRuntimeRemoteDebuggingServer;
class BrowserContextAdapter;
class URLRequestContextFactory;

class AppRuntimeBrowserMainParts : public content::BrowserMainParts {
 public:
  AppRuntimeBrowserMainParts(
      URLRequestContextFactory* url_request_context_factory,
      content::BrowserContext* p = nullptr);
  ~AppRuntimeBrowserMainParts() override;

  void AddParts(AppRuntimeBrowserMainExtraParts* parts);
  int DevToolsPort() const;
  void EnableDevTools();
  void DisableDevTools();

  int PreEarlyInitialization() override;
  void ToolkitInitialized() override;
  void PreMainMessageLoopRun() override;
  bool MainMessageLoopRun(int* result_code) override;
  void PostMainMessageLoopRun() override;

  BrowserContextAdapter* GetDefaultBrowserContext() const {
    return browser_context_adapter_.get();
  }

  URLRequestContextFactory* GetURLRequestContextFactory() const {
    return url_request_context_factory_;
  }

  void ArmWatchdog(content::BrowserThread::ID thread, watchdog::Watchdog* watchdog);

 private:
  std::unique_ptr<watchdog::Watchdog> ui_watchdog_;
  std::unique_ptr<watchdog::Watchdog> io_watchdog_;

  bool dev_tools_enabled_ = false;
  void CreateOSCryptConfig();

  std::unique_ptr<BrowserContextAdapter> browser_context_adapter_;

  URLRequestContextFactory* const url_request_context_factory_;

#if defined(ENABLE_PLUGINS)
  std::unique_ptr<AppRuntimePluginServiceFilter> plugin_service_filter_;
#endif
  std::vector<AppRuntimeBrowserMainExtraParts*> app_runtime_extra_parts_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_APP_RUNTIME_BROWSER_MAIN_PARTS_H_
