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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_

#include <string>
#include <vector>

#include "base/macros.h"

namespace app_runtime {

class AppRuntimeBrowserContext;
class URLRequestContextFactory;
struct ProxySettings;

class BrowserContextAdapter {
 public:
  BrowserContextAdapter(const std::string& storage_name,
                        URLRequestContextFactory* url_request_context_factory);
  virtual ~BrowserContextAdapter();

  static BrowserContextAdapter* GetDefaultContext();

  AppRuntimeBrowserContext* GetBrowserContext() const;

  std::string GetStorageName() const;

  void AppendExtraWebSocketHeader(const std::string& key,
                                  const std::string& value);

  void SetProxyServer(const ProxySettings& proxy_settings);

  void FlushCookieStore();

 private:
  std::string storage_name_;
  AppRuntimeBrowserContext* browser_context_;

  DISALLOW_COPY_AND_ASSIGN(BrowserContextAdapter);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_CONTEXT_ADAPTER_H_
