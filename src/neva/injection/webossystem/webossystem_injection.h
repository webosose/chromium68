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

#ifndef NEVA_INJECTION_WEBOSSYSTEM_WEBOSSYSTEM_INJECTION_H_
#define NEVA_INJECTION_WEBOSSYSTEM_WEBOSSYSTEM_INJECTION_H_

#include "base/compiler_specific.h"
#include "neva/injection/common/public/renderer/injection_data_manager.h"
#include "neva/injection/webossystem/webossystem_export.h"
#include "v8/include/v8.h"

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

#endif  // NEVA_INJECTION_WEBOSSYSTEM_WEBOSSYSTEM_INJECTION_H_
