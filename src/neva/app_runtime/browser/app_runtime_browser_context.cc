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

#include "neva/app_runtime/browser/app_runtime_browser_context.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/files/file_path.h"
#include "browser/app_runtime_browser_context_adapter.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/common/content_switches.h"
#include "net/base/layered_network_delegate.h"
#include "net/http/http_network_session.h"
#include "net/http/http_request_headers.h"
#include "net/proxy_resolution/proxy_config_service.h"
#include "net/proxy_resolution/proxy_info.h"
#include "net/url_request/http_user_agent_settings.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "net/cookies/cookie_store.h"
#include "net/proxy_resolution/proxy_resolution_service.h"

namespace app_runtime {

class AppRuntimeBrowserContext::AppRuntimeResourceContext
    : public content::ResourceContext {
 public:
  AppRuntimeResourceContext(
      URLRequestContextFactory* url_request_context_factory)
      : url_request_context_factory_(url_request_context_factory) {}
  ~AppRuntimeResourceContext() override {}

  // ResourceContext implementation
  net::HostResolver* GetHostResolver() override {
    return url_request_context_factory_->GetMainGetter()
        ->GetURLRequestContext()
        ->host_resolver();
  }

  net::URLRequestContext* GetRequestContext() override {
    return url_request_context_factory_->GetMainGetter()
        ->GetURLRequestContext();
  }

 private:
  URLRequestContextFactory* url_request_context_factory_;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeResourceContext);
};

class AppRuntimeContextNetworkDelegate : public net::LayeredNetworkDelegate {
 public:
  AppRuntimeContextNetworkDelegate(
      std::unique_ptr<net::NetworkDelegate> orig_delegate)
      : net::LayeredNetworkDelegate(std::move(orig_delegate)){};

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value) {
    extra_websocket_headers_.insert(std::make_pair(key, value));
  }

 private:
  void OnBeforeSendHeadersInternal(
      net::URLRequest* request,
      const net::ProxyInfo& proxy_info,
      const net::ProxyRetryInfoMap& proxy_retry_info,
      net::HttpRequestHeaders* headers) override {
    for (const auto& pair : extra_websocket_headers_)
      headers->SetHeader(pair.first, pair.second);
  };

  std::map<std::string, std::string> extra_websocket_headers_;
};

AppRuntimeBrowserContext::AppRuntimeBrowserContext(
    const BrowserContextAdapter* adapter,
    URLRequestContextFactory* url_request_context_factory)
    : adapter_(adapter),
      url_request_context_factory_(url_request_context_factory),
      resource_context_(
          new AppRuntimeResourceContext(url_request_context_factory)) {
  BrowserContext::Initialize(this, GetPath());
}

AppRuntimeBrowserContext::~AppRuntimeBrowserContext() {}

base::FilePath AppRuntimeBrowserContext::GetPath() const {
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kUserDataDir)) {
    base::FilePath path = cmd_line->GetSwitchValuePath(kUserDataDir);
    return path.Append(adapter_->GetStorageName());
  }

  return base::FilePath();
}

bool AppRuntimeBrowserContext::IsOffTheRecord() const {
  return false;
}

net::URLRequestContextGetter* AppRuntimeBrowserContext::CreateRequestContext(
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  net::URLRequestContextGetter* ret =
      url_request_context_factory_->CreateMainGetter(
          this, protocol_handlers, std::move(request_interceptors));
  return ret;
}

net::URLRequestContextGetter*
AppRuntimeBrowserContext::CreateRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  return nullptr;
}

net::URLRequestContextGetter*
AppRuntimeBrowserContext::CreateMediaRequestContext() {
  return url_request_context_factory_->GetMediaGetter();
}

net::URLRequestContextGetter*
AppRuntimeBrowserContext::CreateMediaRequestContextForStoragePartition(
    const base::FilePath& partition_path,
    bool in_memory) {
  return nullptr;
}

content::ResourceContext* AppRuntimeBrowserContext::GetResourceContext() {
  return resource_context_.get();
}

content::DownloadManagerDelegate*
AppRuntimeBrowserContext::GetDownloadManagerDelegate() {
  return nullptr;
}

content::BrowserPluginGuestManager*
AppRuntimeBrowserContext::GetGuestManager() {
  return nullptr;
}

storage::SpecialStoragePolicy*
AppRuntimeBrowserContext::GetSpecialStoragePolicy() {
  return nullptr;
}

content::PushMessagingService*
AppRuntimeBrowserContext::GetPushMessagingService() {
  return nullptr;
}

content::SSLHostStateDelegate*
AppRuntimeBrowserContext::GetSSLHostStateDelegate() {
  return nullptr;
}

std::unique_ptr<content::ZoomLevelDelegate>
AppRuntimeBrowserContext::CreateZoomLevelDelegate(const base::FilePath&) {
  return nullptr;
}

content::PermissionManager* AppRuntimeBrowserContext::GetPermissionManager() {
  return nullptr;
}

content::BackgroundFetchDelegate* AppRuntimeBrowserContext::GetBackgroundFetchDelegate() {
  return nullptr;
}

content::BackgroundSyncController*
AppRuntimeBrowserContext::GetBackgroundSyncController() {
  return nullptr;
}

content::BrowsingDataRemoverDelegate*
AppRuntimeBrowserContext::GetBrowsingDataRemoverDelegate() {
  return nullptr;
}

void AppRuntimeBrowserContext::SetProxyServer(const std::string& ip,
                                              const std::string& port,
                                              const std::string& name,
                                              const std::string& password) {
  url_request_context_factory_->SetProxyServer(ip, port, name, password);
}

void AppRuntimeBrowserContext::AppendExtraWebSocketHeader(
    const std::string& key,
    const std::string& value) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&AppRuntimeBrowserContext::AppendExtraWebSocketHeaderIO,
                 base::Unretained(this), key, value));
}

void AppRuntimeBrowserContext::AppendExtraWebSocketHeaderIO(
    const std::string& key,
    const std::string& value) {
  if (!network_delegate_.get()) {
    network_delegate_.reset(new AppRuntimeContextNetworkDelegate(
        std::unique_ptr<net::NetworkDelegate>(
            GetResourceContext()->GetRequestContext()->network_delegate())));
    GetResourceContext()->GetRequestContext()->set_network_delegate(
        network_delegate_.get());
  }

  network_delegate_->AppendExtraWebSocketHeader(key, value);
}

void AppRuntimeBrowserContext::FlushCookieStore() {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(&AppRuntimeBrowserContext::FlushCookieStoreIO,
                 base::Unretained(this)));
}

void AppRuntimeBrowserContext::FlushCookieStoreIO() {
  net::CookieStore* cookie_store =
      GetResourceContext()->GetRequestContext()->cookie_store();
  if (cookie_store)
    cookie_store->FlushStore(base::Closure());
}

}  // namespace app_runtime
