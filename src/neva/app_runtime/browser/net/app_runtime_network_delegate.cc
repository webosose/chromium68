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

#include "neva/app_runtime/browser/net/app_runtime_network_delegate.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "net/base/net_errors.h"
#include "net/url_request/url_request.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"

namespace app_runtime {

AppRuntimeNetworkDelegate::AppRuntimeNetworkDelegate() {
}

AppRuntimeNetworkDelegate::~AppRuntimeNetworkDelegate() {}

bool AppRuntimeNetworkDelegate::OnCanAccessFile(
    const net::URLRequest& request,
    const base::FilePath& original_path,
    const base::FilePath& absolute_path) const {
  // By default the access to files is denied. Use the app_runtime::SetNetworkDelegate to
  // shape out own file access logic.
  return base::CommandLine::ForCurrentProcess()->HasSwitch(kAllowFileAccess);
}

int AppRuntimeNetworkDelegate::OnBeforeURLRequest(
    net::URLRequest* request,
    const net::CompletionCallback& callback,
    GURL* new_url) {
  if (GetAppRuntimeContentBrowserClient()->DoNotTrack()) {
    static const char DNTHeader[] = "DNT";
    request->SetExtraRequestHeaderByName(DNTHeader, "1", true);
  }
  return net::OK;
}

}  // namespace app_runtime
