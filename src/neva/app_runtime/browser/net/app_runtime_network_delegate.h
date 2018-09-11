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

#ifndef NEVA_APP_RUNTIME_BROWSER_NET_APP_RUNTIME_NETWORK_DELEGATE_H_
#define NEVA_APP_RUNTIME_BROWSER_NET_APP_RUNTIME_NETWORK_DELEGATE_H_

#include "net/base/completion_callback.h"
#include "net/base/network_delegate_impl.h"

namespace app_runtime {

class AppRuntimeNetworkDelegate : public net::NetworkDelegateImpl {
 public:
  AppRuntimeNetworkDelegate();
  ~AppRuntimeNetworkDelegate() override;

 private:
  // net::NetworkDelegate implementation
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& original_path,
                       const base::FilePath& absolute_path) const override;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_NET_APP_RUNTIME_NETWORK_DELEGATE_H_
