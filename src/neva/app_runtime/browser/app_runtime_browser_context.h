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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_

#include "content/public/browser/browser_context.h"

namespace app_runtime {

class AppRuntimeContextNetworkDelegate;
class BrowserContextAdapter;
class URLRequestContextFactory;

class AppRuntimeBrowserContext : public content::BrowserContext {
 public:
  static AppRuntimeBrowserContext* Get();
  AppRuntimeBrowserContext(
      const BrowserContextAdapter* adapter,
      URLRequestContextFactory* url_request_context_factory);
  ~AppRuntimeBrowserContext() override;
  base::FilePath GetPath() const override;
  bool IsOffTheRecord() const override;

  net::URLRequestContextGetter* CreateRequestContext(
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors) override;
  net::URLRequestContextGetter* CreateMediaRequestContext() override;
  net::URLRequestContextGetter* CreateMediaRequestContextForStoragePartition(
      const base::FilePath& partition_path,
      bool in_memory) override;

  content::ResourceContext* GetResourceContext() override;
  content::DownloadManagerDelegate* GetDownloadManagerDelegate() override;
  content::BrowserPluginGuestManager* GetGuestManager() override;
  storage::SpecialStoragePolicy* GetSpecialStoragePolicy() override;
  content::PushMessagingService* GetPushMessagingService() override;
  content::SSLHostStateDelegate* GetSSLHostStateDelegate() override;
  std::unique_ptr<content::ZoomLevelDelegate> CreateZoomLevelDelegate(
      const base::FilePath&) override;
  content::PermissionManager* GetPermissionManager() override;
  content::BackgroundFetchDelegate* GetBackgroundFetchDelegate() override;
  content::BackgroundSyncController* GetBackgroundSyncController() override;
  content::BrowsingDataRemoverDelegate* GetBrowsingDataRemoverDelegate() override;

  void SetProxyServer(const std::string& proxyIp,
                      const std::string& proxyPort,
                      const std::string& proxyUsername,
                      const std::string& proxyPassword);
  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);

  void FlushCookieStore();

 private:
  class AppRuntimeResourceContext;

  void AppendExtraWebSocketHeaderIO(const std::string& key,
                                    const std::string& value);
  void FlushCookieStoreIO();

  const BrowserContextAdapter* adapter_;
  URLRequestContextFactory* const url_request_context_factory_;
  std::unique_ptr<AppRuntimeResourceContext> resource_context_;
  std::unique_ptr<AppRuntimeContextNetworkDelegate> network_delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeBrowserContext);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_H_
