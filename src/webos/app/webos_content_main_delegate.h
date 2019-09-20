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

#ifndef WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_
#define WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_

#include <memory>

#include "content/public/renderer/content_renderer_client.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#if defined(OS_WEBOS)
#include "webos/browser/webos_luna_service_delegate.h"
#endif
#include "webos/common/webos_content_client.h"

namespace app_runtime {
class AppRuntimeContentBrowserClient;
}  // namespace app_runtime

namespace webos {

class WebOSContentMainDelegate : public app_runtime::AppRuntimeMainDelegate {
 public:
  WebOSContentMainDelegate();

  // content::ContentMainDelegate implementation:
  bool BasicStartupComplete(int* exit_code) override;
  void SetBrowserStartupCallback(base::Closure startup_callback) {
    startup_callback_ = startup_callback;
  }
  content::ContentRendererClient* CreateContentRendererClient() override;

 protected:
  WebOSContentClient content_client_;
  std::unique_ptr<content::ContentRendererClient> content_renderer_client_;
#if defined(OS_WEBOS)
  std::unique_ptr<WebOSLunaServiceDelegate> webos_luna_service_delegate_;
#endif
  base::Closure startup_callback_;
};

}  // namespace webos

#endif  // WEBOS_APP_WEBOS_CONTENT_MAIN_DELEGATE_H_
