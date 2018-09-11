// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_DEVTOOLS_MANAGER_DELEGATE_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_DEVTOOLS_MANAGER_DELEGATE_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/devtools_manager_delegate.h"

namespace content {
class BrowserContext;
}

namespace app_runtime {
using namespace content;

class AppRuntimeDevToolsManagerDelegate : public DevToolsManagerDelegate {
 public:
  static void StartHttpHandler(BrowserContext* browser_context);
  static void StopHttpHandler();
  static int GetHttpHandlerPort();

  explicit AppRuntimeDevToolsManagerDelegate();
  ~AppRuntimeDevToolsManagerDelegate() override;

  // DevToolsManagerDelegate implementation.
  std::string GetDiscoveryPageHTML() override;
  bool HasBundledFrontendResources() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(AppRuntimeDevToolsManagerDelegate);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_DEVTOOLS_MANAGER_DELEGATE_H_
