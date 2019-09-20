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

#include "webos/app/webos_main.h"

#include "content/public/app/content_main.h"
#if defined(OS_WEBOS)
#include "content/public/browser/runtime_delegate_webos.h"
#endif
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#include "webos/app/webos_content_main_delegate.h"
#include "webos/browser/net/webos_network_delegate.h"
#include "webos/common/webos_runtime_delegate.h"
#include "webos/public/runtime.h"

namespace webos {

WebOSMain::WebOSMain(WebOSMainDelegate* delegate)
    : delegate_(delegate) {}


int WebOSMain::Run(int argc, const char** argv) {
  webos::WebOSContentMainDelegate main_delegate;
  app_runtime::SetNetworkDelegate(new WebOSNetworkDelegate());
  main_delegate.SetBrowserStartupCallback(
      base::Bind(&WebOSMainDelegate::AboutToCreateContentBrowserClient,
                 base::Unretained(delegate_)));
  content::ContentMainParams params(&main_delegate);

  params.argc = argc;
  params.argv = argv;

#if defined(OS_WEBOS)
  content::SetRuntimeDelegateWebOS(new webos::WebOSRuntimeDelegate());
#endif

  return content::ContentMain(params);
}

}  // namespace webos
