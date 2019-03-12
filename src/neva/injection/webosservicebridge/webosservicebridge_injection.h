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

#ifndef NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_INJECTION_H_
#define NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_INJECTION_H_

#include <set>

#include "base/compiler_specific.h"
#include "neva/injection/webosservicebridge/luna_service_mgr.h"
#include "neva/injection/webosservicebridge/webosservicebridge_export.h"
#include "v8/include/v8.h"

namespace blink {
class WebLocalFrame;
}

namespace gin {
class Arguments;
}

namespace extensions_v8 {

class WEBOSSERVICEBRIDGE_EXPORT WebOSServiceBridgeInjectionExtension {
 public:
  static void Install(blink::WebLocalFrame* frame);
  static const char kWebOSServiceBridgeInjectionName[];
};

class WebOSServiceBridgeInjection : public LunaServiceManagerListener {
 public:
  static const char kOnServiceCallbackMethodName[];
  static const char kCallMethodName[];
  static const char kCancelMethodName[];

  // To handle luna call in webOSSystem.onclose callback
  static std::set<WebOSServiceBridgeInjection*> waiting_responses_;
  static bool is_closing_;

  static void WebOSServiceBridgeConstructorCallback(gin::Arguments* args);

  static void CallService(gin::Arguments* args);
  static void CancelServiceCall(gin::Arguments* args);
  static void OnServiceCallback(const gin::Arguments& args);

  WebOSServiceBridgeInjection() { Init(); }
  ~WebOSServiceBridgeInjection();

  void Init();
  void Call();
  void Call(const char* uri, const char* payload);
  void Cancel();
  virtual void ServiceResponse(const char* body);

 private:
  void SetupIdentifier();
  void CloseNotify();

  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data);
  static v8::Local<v8::ObjectTemplate> MakeRequestTemplate(
      v8::Isolate* isolate);

  static v8::Persistent<v8::ObjectTemplate> request_template_;
  v8::Persistent<v8::Object> object_;
  bool canceled_ = false;
  std::string identifier_;
  std::shared_ptr<LunaServiceManager> lsm_;
};

}  // namespace extensions_v8

#endif  // NEVA_INJECTION_WEBOSSERVICEBRIDGE_WEBOSSERVICEBRIDGE_INJECTION_H_
