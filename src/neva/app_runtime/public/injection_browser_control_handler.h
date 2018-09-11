// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_PUBLIC_INJECTION_BROWSER_CONTROL_HANDLER_H_
#define NEVA_APP_RUNTIME_PUBLIC_INJECTION_BROWSER_CONTROL_HANDLER_H_

#include <string>
#include <memory>
#include <vector>

namespace app_runtime {

class WebViewBase;

// We temporarily have to use this class while there are two different
// implementations WebViewBase in appruntime and webos
namespace internals {
class WebViewBaseInternals;
}  // namespace internals

class InjectionBrowserControlHandlerDelegate {
public:
  virtual void OnBrowserControlCommand(
      const std::string& command,
      const std::vector<std::string>& arguments) = 0;

  virtual void OnBrowserControlFunction(
      const std::string& command,
      const std::vector<std::string>& arguments,
      std::string* result) = 0;
};

class InjectionBrowserControlHandler {
 public:
  // We temporarily have to use this class while there are two different
  // implementations WebViewBase in appruntime and webos
  // explicit InjectionBrowserControlHandler(WebViewBase* webviewbase);

  explicit InjectionBrowserControlHandler(
      internals::WebViewBaseInternals* webview_basei_internals);
  ~InjectionBrowserControlHandler();

  void SetDelegate(InjectionBrowserControlHandlerDelegate* delegate);
  InjectionBrowserControlHandlerDelegate* GetDelegate() const;

 private:
  class InjectionBrowserControlObserver;
  std::unique_ptr<InjectionBrowserControlObserver> injection_observer_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_INJECTION_BROWSER_CONTROL_HANDLER_H_
