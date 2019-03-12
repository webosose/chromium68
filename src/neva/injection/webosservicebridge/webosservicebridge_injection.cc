// Copyright 2019 LG Electronics, Inc.
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

#include "injection/webosservicebridge/webosservicebridge_injection.h"

#include "base/bind.h"
#include "base/json/json_writer.h"
#include "base/logging_pmlog.h"
#include "base/values.h"
#include "gin/arguments.h"
#include "gin/dictionary.h"
#include "gin/function_template.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "gin/wrappable.h"
#include "injection/common/public/renderer/injection_base.h"
#include "injection/common/public/renderer/injection_webos.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include <string>
#include <time.h>

namespace extensions_v8 {

const char
    WebOSServiceBridgeInjectionExtension::kWebOSServiceBridgeInjectionName[] =
        "v8/webosservicebridge";

const char kMethodInvocationAsConstructorDisallowed[] =
    "webOS service bridge function can't be invoked as a constructor";

const char WebOSServiceBridgeInjection::kOnServiceCallbackMethodName[] =
    "onservicecallback";
const char WebOSServiceBridgeInjection::kCallMethodName[] = "call";
const char WebOSServiceBridgeInjection::kCancelMethodName[] = "cancel";

// To handle luna call in webOSSystem.onclose callback
bool WebOSServiceBridgeInjection::is_closing_ = false;
std::set<WebOSServiceBridgeInjection*>
    WebOSServiceBridgeInjection::waiting_responses_;

v8::Persistent<v8::ObjectTemplate>
    WebOSServiceBridgeInjection::request_template_;

// static
void WebOSServiceBridgeInjection::CallService(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  if (!args->GetHolder(&holder))
    return;
  v8::Local<v8::External> wrap =
      v8::Local<v8::External>::Cast(holder->GetInternalField(0));
  WebOSServiceBridgeInjection* self =
      static_cast<WebOSServiceBridgeInjection*>(wrap->Value());

  if (!self)
    return;

  if (args->Length() < 2) {
    self->Call();
    return;
  }

  v8::Local<v8::Value> url;
  v8::Local<v8::Value> params;

  if (!args->GetNext(&url) || !args->GetNext(&params) || !url->IsString() ||
      !params->IsString())
    self->Call();
  else
    self->Call(gin::V8ToString(url).c_str(), gin::V8ToString(params).c_str());
}

// static
void WebOSServiceBridgeInjection::CancelServiceCall(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  if (!args->GetHolder(&holder))
    return;
  v8::Local<v8::External> wrap =
      v8::Local<v8::External>::Cast(holder->GetInternalField(0));
  WebOSServiceBridgeInjection* self =
      static_cast<WebOSServiceBridgeInjection*>(wrap->Value());

  if (self)
    self->Cancel();
}

void WebOSServiceBridgeInjection::OnServiceCallback(
    const gin::Arguments& args) {
  // NOTIMPLEMENTED();
}

// static
void WebOSServiceBridgeInjection::FirstWeakCallback(
    const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data) {
  WebOSServiceBridgeInjection* bridge = data.GetParameter();
  bridge->object_.Reset();
  data.SetSecondPassCallback(SecondWeakCallback);
}

// static
void WebOSServiceBridgeInjection::SecondWeakCallback(
    const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data) {
  WebOSServiceBridgeInjection* bridge = data.GetParameter();
  delete bridge;
}

WebOSServiceBridgeInjection::~WebOSServiceBridgeInjection() {
  Cancel();

  if (!object_.IsEmpty())
    object_.Reset();
}

void WebOSServiceBridgeInjection::SetupIdentifier() {
  const char* data = "webOSSystem.getIdentifier()";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, data);
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  v8::Local<v8::Value> result = script->Run();
  if (!result.IsEmpty() && result->IsString())
    identifier_ = *v8::String::Utf8Value(result);
}

void WebOSServiceBridgeInjection::CloseNotify() {
  if (!WebOSServiceBridgeInjection::is_closing_ ||
      !WebOSServiceBridgeInjection::waiting_responses_.empty())
    return;

  const char* data = "webOSSystem.onCloseNotify(\"didRunOnCloseCallback\")";
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::String> source = v8::String::NewFromUtf8(isolate, data);
  v8::Local<v8::Script> script = v8::Script::Compile(source);
  script->Run();
}

void WebOSServiceBridgeInjection::Init() {
  SetupIdentifier();
  lsm_ = LunaServiceManager::GetManager(identifier_);
}

void WebOSServiceBridgeInjection::Call() {
  Call("", "");
}

void WebOSServiceBridgeInjection::Call(const char* uri, const char* payload) {
  if (identifier_.empty())
    return;

  if (!lsm_)
    return;

  if (WebOSServiceBridgeInjection::is_closing_) {
    auto ret = waiting_responses_.insert(this);
    if (!lsm_->Call(uri, payload, this))
      waiting_responses_.erase(ret.first);
  } else {
    lsm_->Call(uri, payload, this);
  }
}

void WebOSServiceBridgeInjection::Cancel() {
  if (!lsm_ || canceled_)
    return;

  canceled_ = true;
  if (GetListenerToken())
    lsm_->Cancel(this);

  if (WebOSServiceBridgeInjection::is_closing_)
    waiting_responses_.erase(this);
}

void WebOSServiceBridgeInjection::ServiceResponse(const char* body) {
  if (object_.IsEmpty() || object_.IsNearDeath())
    return;

  bool shouldCallCloseNotify = false;
  if (WebOSServiceBridgeInjection::is_closing_) {
    waiting_responses_.erase(this);
    if (waiting_responses_.empty())
      shouldCallCloseNotify = true;
  }

  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Object> recv = v8::Local<v8::Object>::New(isolate, object_);
  v8::Context::Scope context_scope(recv->CreationContext());
  v8::Local<v8::String> callback_key(v8::String::NewFromUtf8(
      isolate, WebOSServiceBridgeInjection::kOnServiceCallbackMethodName));

  if (!recv->Has(callback_key))
    return;

  v8::Local<v8::Value> func_value = recv->Get(callback_key);
  if (!func_value->IsFunction())
    return;

  v8::Local<v8::Function> func = v8::Local<v8::Function>::Cast(func_value);

  const int argc = 1;
  v8::Local<v8::Value> argv[] = {v8::String::NewFromUtf8(isolate, body)};
  func->Call(recv, argc, argv);

  if (shouldCallCloseNotify) {
    // This function should be executed after context_scope(context)
    // Unless there will be libv8.so crash
    CloseNotify();
  }
}

// static
v8::Local<v8::ObjectTemplate> WebOSServiceBridgeInjection::MakeRequestTemplate(
    v8::Isolate* isolate) {
  v8::EscapableHandleScope handle_scope(isolate);
  v8::Local<v8::ObjectTemplate> instance_templ =
      gin::ObjectTemplateBuilder(isolate)
          .SetMethod(WebOSServiceBridgeInjection::kCallMethodName,
                     &WebOSServiceBridgeInjection::CallService)
          .SetMethod(WebOSServiceBridgeInjection::kCancelMethodName,
                     &WebOSServiceBridgeInjection::CancelServiceCall)
          .SetMethod(WebOSServiceBridgeInjection::kOnServiceCallbackMethodName,
                     &WebOSServiceBridgeInjection::OnServiceCallback)
          .Build();

  return handle_scope.Escape(instance_templ);
}

// static
void WebOSServiceBridgeInjection::WebOSServiceBridgeConstructorCallback(
    gin::Arguments* args) {
  if (!args->IsConstructCall()) {
    args->isolate()->ThrowException(v8::Exception::Error(gin::StringToV8(
        args->isolate(), kMethodInvocationAsConstructorDisallowed)));
    return;
  }

  v8::Isolate* isolate = args->isolate();
  v8::HandleScope handle_scope(isolate);

  if (request_template_.IsEmpty()) {
    v8::Handle<v8::ObjectTemplate> object_templ = MakeRequestTemplate(isolate);
    request_template_.Reset(isolate, object_templ);
  }

  v8::Handle<v8::ObjectTemplate> instance_template =
      v8::Local<v8::ObjectTemplate>::New(isolate, request_template_);

  v8::Local<v8::Object> instance = instance_template->NewInstance();

  WebOSServiceBridgeInjection* p = new WebOSServiceBridgeInjection();
  p->object_.Reset(isolate, instance);
  p->object_.SetWeak(p, WebOSServiceBridgeInjection::FirstWeakCallback,
                     v8::WeakCallbackType::kParameter);

  instance->SetInternalField(0, v8::External::New(isolate, p));

  args->Return(instance);
}

void WebOSServiceBridgeInjectionExtension::Install(
    blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  v8::Local<v8::FunctionTemplate> templ =
      gin::CreateFunctionTemplateForConstructorBehavior(
          isolate, base::Bind(&WebOSServiceBridgeInjection::
                                  WebOSServiceBridgeConstructorCallback));

  global->Set(gin::StringToSymbol(isolate, "webOSServiceBridge"),
              templ->GetFunction());

  const char kExtraObjectsForWebOSServiceBridgeInjectionAPI[] =
      // Place this code always at the end of injection API
      // Support PalmServiceBridge for backward compatibility
      "var PalmServiceBridge;"
      "if (typeof(PalmServiceBridge) == 'undefined') {"
      "  PalmServiceBridge = webOSServiceBridge;"
      "};";

  v8::Script::CompileExtraInjections(
      isolate,
      gin::StringToV8(isolate, kExtraObjectsForWebOSServiceBridgeInjectionAPI),
      WebOSServiceBridgeInjectionExtension::kWebOSServiceBridgeInjectionName);
}

}  // namespace extensions_v8
