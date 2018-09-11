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

#include "injection/common/public/renderer/injection_base.h"

#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace extensions_v8 {

InjectionBase::InjectionBase() {}

void InjectionBase::Dispatch(blink::WebLocalFrame* frame,
                             const blink::WebString& script) {
  if (!frame) {
    return;
  }

  frame->ExecuteScript(blink::WebScriptSource(script));
}

void InjectionBase::NotImplemented(
    const InjectionWrapper::CallbackInfoValue& args) {
#ifdef USE_BASE_LOGGING
  LOG(ERROR) << "error: 'NotImplemented' function call";
#endif
}
}  // namespace extensions_v8
