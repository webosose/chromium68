// Copyright (c) 2014-2019 LG Electronics, Inc.
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

#ifndef CHROME_INJECTION_WEBOSSYSTEM_INJECTION_H_
#define CHROME_INJECTION_WEBOSSYSTEM_INJECTION_H_

#include "base/compiler_specific.h"
#include "injection/common/public/renderer/injection_data_manager.h"
#include "v8/include/v8.h"

#if defined(COMPONENT_BUILD)

#if defined(WEBOSSYSTEM_IMPLEMENTATION)
#define WEBOSSYSTEM_EXPORT __attribute__((visibility("default")))
#else
#define WEBOSSYSTEM_EXPORT
#endif  // defined(WEBOSSYSTEM_IMPLEMENTATION)

#else
#define WEBOSSYSTEM_EXPORT
#endif  // defined(COMPONENT_BUILD)

namespace blink {
class WebLocalFrame;
}

namespace extensions_v8 {

class WEBOSSYSTEM_EXPORT WebOSSystemInjectionExtension {
 public:
  static void Install(blink::WebLocalFrame* frame);
  static const char kWebOSSystemInjectionName[];

 private:
  static v8::Local<v8::Object> CreateWebOSSystemObject(
      v8::Isolate* isolate,
      v8::Local<v8::Context> context,
      v8::Local<v8::Object> global);
};

}  // namespace extensions_v8

#endif  // CHROME_INJECTION_WEBOSSYSTEM_INJECTION_H_
