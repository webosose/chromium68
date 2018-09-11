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

#ifndef INJECTION_BROWSER_CONTROL_BROWSER_CONTROL_INJECTION_H_
#define INJECTION_BROWSER_CONTROL_BROWSER_CONTROL_INJECTION_H_

#include <string>

#include "base/compiler_specific.h"

#if defined(COMPONENT_BUILD)
#define BROWSER_CONTROL_INJECTION_EXPORT __attribute__((visibility("default")))
#else
#define BROWSER_CONTROL_INJECTION_EXPORT
#endif  // defined(COMPONENT_BUILD)

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace extensions_v8 {

class InjectionWrapper;

class BROWSER_CONTROL_INJECTION_EXPORT BrowserControlInjectionExtension {
 public:
  static InjectionWrapper* Get();
  static std::string GetNavigatorExtensionScript();

  static void DispatchValueChanged(blink::WebLocalFrame* web_frame,
                                   const std::string& val);
  static void InstallExtension(blink::WebLocalFrame *web_frame);

  static const char kInjectionName[];
};

}  // namespace extensions_v8

#endif  // INJECTION_BROWSER_CONTROL_BROWSER_CONTROL_INJECTION_H_
