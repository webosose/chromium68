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

#include "webview_profile.h"

#include "base/time/time.h"
#include "browser/browsing_data/browsing_data_remover.h"
#include "browser/app_runtime_browser_context_adapter.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"

namespace app_runtime {

WebViewProfile::WebViewProfile(const std::string& storage_name)
    // FIXME: memory leak. When we create WebViewProfile with default context,
    // we must not destroy this context on dtor. In this ctor we create new
    // BrowserContextAdapter but we never destroy it.
    : browser_context_adapter_(
          new BrowserContextAdapter(storage_name, GetAppRuntimeContentBrowserClient()
                                                    ->GetMainParts()
                                                    ->GetURLRequestContextFactory())) {}

WebViewProfile* WebViewProfile::GetDefaultProfile() {
  static WebViewProfile* profile =
      new WebViewProfile(BrowserContextAdapter::GetDefaultContext());
  return profile;
}

WebViewProfile::WebViewProfile(BrowserContextAdapter* adapter)
    : browser_context_adapter_(adapter) {}

BrowserContextAdapter* WebViewProfile::GetBrowserContextAdapter() const {
  return browser_context_adapter_;
}

void WebViewProfile::SetProxyServer(const std::string& ip,
                    const std::string& port,
                    const std::string& username,
                    const std::string& password) {
  browser_context_adapter_->SetProxyServer(ip, port, username, password);
}

void WebViewProfile::AppendExtraWebSocketHeader(const std::string& key,
                                                const std::string& value) {
  browser_context_adapter_->AppendExtraWebSocketHeader(key, value);
}

void WebViewProfile::RemoveBrowsingData(int remove_browsing_data_mask) {
  BrowsingDataRemover* remover = BrowsingDataRemover::GetForBrowserContext(
      browser_context_adapter_->GetBrowserContext());
  remover->Remove(BrowsingDataRemover::Unbounded(), remove_browsing_data_mask);
}

void WebViewProfile::FlushCookieStore() {
  browser_context_adapter_->FlushCookieStore();
}

}  // namespace app_runtime
