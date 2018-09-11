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

#include "injection/browser_control/browser_control_injection.h"

#include <sstream>

#include "base/bind.h"
#include "base/logging.h"
#include "base/macros.h"
#include "content/public/renderer/render_frame.h"
#include "injection/common/public/renderer/injection_browser_control_base.h"
#include "injection/common/util/arguments_checking.h"
#include "injection/common/wrapper/injection_wrapper.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace {

// cached value for BrowserControlInjection::GetValue function
static std::string app_value = "null";

}  // anonymous namespace

namespace extensions_v8 {

const char BrowserControlInjectionExtension::kInjectionName[] =
    "v8/browser_control";

const char kBrowserControlInjectionAPI[] =
R"JS(
function BrowserControl() {
  if (typeof(navigator.browser_control) != 'undefined') {
    throw new TypeError("Illegal constructor");
  };

  function init() {
    var _obj = {};
    native function BrowserControlInitialize();
    BrowserControlInitialize.call(_obj);
    return _obj
  }

  // It allows to initialize object BrowserControl and get access to it from navigator
  function get() {
    var value = init.call(this)
    Object.defineProperty(this, "browser_control", {
      value: value,
      configurable: false,
    })
        return value
  }

  return {
    get: get,
    configurable: true,
  };
};
)JS";

static const char kInstallBrowserControlInjection[] =
R"JS(
if (typeof BrowserControl !== 'undefined' && navigator) {
  if (document && (typeof(navigator.browser_control) == 'undefined')){
    Object.defineProperty(navigator, "browser_control", BrowserControl());
  };
};
)JS";

class BrowserControlInjection : public InjectionWrapper
                              , public InjectionBrowserControlBase {
 public:
  BrowserControlInjection();

  static void BrowserControlInitialize(
      const InjectionWrapper::CallbackInfoValue& args);
  static void DoCallFunction(const InjectionWrapper::CallbackInfoValue& args);
  static void DoSendCommand(const InjectionWrapper::CallbackInfoValue& args);
};

BrowserControlInjection::BrowserControlInjection()
    : InjectionWrapper(BrowserControlInjectionExtension::kInjectionName,
                       kBrowserControlInjectionAPI) {
  IW_ADDCALLBACK(BrowserControlInitialize);
}

// static
void BrowserControlInjection::BrowserControlInitialize(
    const InjectionWrapper::CallbackInfoValue& args) {
  // Next code should be wrapper with neva InjectionWrapper
  // methods and types
  v8::Isolate* isolate = args.GetIsolate();
  args.This()->Set(
      v8::String::NewFromUtf8(isolate, "SendCommand"),
      v8::Function::New(isolate, DoSendCommand));
  args.This()->Set(
      v8::String::NewFromUtf8(isolate, "CallFunction"),
      v8::Function::New(isolate, DoCallFunction));
}

// static
void BrowserControlInjection::DoCallFunction(
    const InjectionWrapper::CallbackInfoValue& args) {
  JSFunctionCallbackInfoValue js_args(args);
  const int length = js_args.Length();
  // At least one argument must always be passed,
  // since this is the name of a function
  if (length == 0)
    return;

  const std::string function_name = *v8::String::Utf8Value(js_args.GetIsolate(),
                                                           args[0]);
  std::vector<std::string> arguments;
  for (int i = 1; i < length; ++i) {
    std::string value;
    if (CheckArguments(args).Argument(i).IsString().BindString(value) ==
        CheckResult::RESULT_VALID)
      arguments.push_back(value);
  }

  std::string result = CallFunction(function_name, arguments);
  JSReturnValue(js_args.GetReturnValue()).Set(
      NewLocalStringFromUtf8(js_args.GetIsolate(), result.c_str()));
}

void BrowserControlInjection::DoSendCommand(
    const InjectionWrapper::CallbackInfoValue& args) {
  JSFunctionCallbackInfoValue js_args(args);
  const int length = js_args.Length();
  // At least one argument must always be passed,
  // since this is the name of a command
  if (length == 0)
    return;

  const std::string command_name = *v8::String::Utf8Value(js_args.GetIsolate(),
                                                          args[0]);
  std::vector<std::string> arguments;
  for (int i = 1; i < length; ++i) {
    std::string value;
    if (CheckArguments(args).Argument(i).IsString().BindString(value) ==
        CheckResult::RESULT_VALID)
      arguments.push_back(value);
  }

  SendCommand(command_name, arguments);
}

// static
InjectionWrapper* BrowserControlInjectionExtension::Get() {
  return new BrowserControlInjection();
}

// static
void BrowserControlInjectionExtension::InstallExtension(
    blink::WebLocalFrame* web_frame) {
  if (web_frame) {
    BrowserControlInjection::Dispatch(web_frame, kInstallBrowserControlInjection);
  }
}

std::string BrowserControlInjectionExtension::GetNavigatorExtensionScript() {
  return kInstallBrowserControlInjection;
}

}  // namespace extensions_v8
