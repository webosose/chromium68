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

#include "injection/webossystem/webossystem_injection.h"

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
#include "luna_service_mgr.h"
#include "base/trace_event/neva/lttng/pmtracer.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include <string>
#include <time.h>

namespace extensions_v8 {

const char WebOSSystemInjectionExtension::kWebOSSystemInjectionName[] =
    "v8/webossystem";

const char kMethodInvocationAsConstructorDisallowed[] =
    "webOS service bridge function can't be invoked as a constructor";

class WebOSSystemDataManager : public InjectionDataManager {
 public:
  WebOSSystemDataManager(const std::string& json);
  ~WebOSSystemDataManager() = default;
  bool GetInitializedStatus() const { return initialized_; }
  void SetInitializedStatus(bool status) { initialized_ = status; }
  void DoInitialize(const std::string& json);

 private:
  static const std::vector<std::string> cached_data_keys_;
  bool initialized_;
};

const std::vector<std::string> WebOSSystemDataManager::cached_data_keys_ = {
    "activityId", "country",      "currentCountryGroup", "deviceInfo",
    "folderPath", "identifier",   "isMinimal",           "launchParams",
    "locale",     "localeRegion", "phoneRegion",         "screenOrientation",
    "timeFormat", "timeZone",     "devicePixelRatio",    "trustLevel"};

WebOSSystemDataManager::WebOSSystemDataManager(const std::string& json)
    : initialized_(false) {
  DoInitialize(json);
}

void WebOSSystemDataManager::DoInitialize(const std::string& json) {
  if (!initialized_ && json != "") {
    Initialize(json, cached_data_keys_);
    initialized_ = true;
  }
}

class WebOSServiceBridgeInjection : public LunaServiceManagerListener {
 public:
  static const char kOnServiceCallbackMethodName[];
  static const char kCallMethodName[];
  static const char kCancelMethodName[];

  // To handle luna call in webOSSystem.onclose callback
  static std::set<WebOSServiceBridgeInjection*> waiting_responses_;
  static bool is_closing_;

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
  friend class WebOSSystemInjection;
  void SetupIdentifier();
  void CloseNotify();

  static void FirstWeakCallback(
      const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data);
  static void SecondWeakCallback(
      const v8::WeakCallbackInfo<WebOSServiceBridgeInjection>& data);

  v8::Persistent<v8::Object> object_;
  bool canceled_ = false;
  std::string identifier_;
  std::shared_ptr<LunaServiceManager> lsm_;
};

const char WebOSServiceBridgeInjection::kOnServiceCallbackMethodName[] =
    "onservicecallback";
const char WebOSServiceBridgeInjection::kCallMethodName[] = "call";
const char WebOSServiceBridgeInjection::kCancelMethodName[] = "cancel";

// To handle luna call in webOSSystem.onclose callback
bool WebOSServiceBridgeInjection::is_closing_ = false;
std::set<WebOSServiceBridgeInjection*>
    WebOSServiceBridgeInjection::waiting_responses_;

// static
void WebOSServiceBridgeInjection::CallService(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  args->GetHolder(&holder);
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

  args->GetNext(&url);
  args->GetNext(&params);

  if (!url->IsString() || !params->IsString())
    self->Call();
  else
    self->Call(gin::V8ToString(url).c_str(), gin::V8ToString(params).c_str());
}

// static
void WebOSServiceBridgeInjection::CancelServiceCall(gin::Arguments* args) {
  v8::Local<v8::Object> holder;
  args->GetHolder(&holder);
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
  v8::Local<v8::Value> result = script->Run();
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

  if (lsm_) {
    if (WebOSServiceBridgeInjection::is_closing_) {
      auto ret = waiting_responses_.insert(this);
      if (!lsm_->Call(uri, payload, this))
        waiting_responses_.erase(ret.first);
    } else
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

class CursorInjection : public gin::Wrappable<CursorInjection> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  class Delegate {
   public:
    virtual std::string CallFunctionName(const char* name) = 0;
    virtual bool SetCursor(gin::Arguments* args) = 0;
  };

  CursorInjection(Delegate* delegate);
  ~CursorInjection() = default;

 private:
  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  bool GetCursorVisibility();
  std::string GetCursorState();
  bool SetCursor(gin::Arguments* args);

  Delegate* delegate_;
};

CursorInjection::CursorInjection(Delegate* delegate) : delegate_(delegate) {}

gin::WrapperInfo CursorInjection::kWrapperInfo = {gin::kEmbedderNativeGin};
gin::ObjectTemplateBuilder CursorInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<CursorInjection>::GetObjectTemplateBuilder(isolate)
      .SetProperty("visibility", &CursorInjection::GetCursorVisibility)
      .SetMethod("getCursorState", &CursorInjection::GetCursorState)
      .SetMethod("setCursor", &CursorInjection::SetCursor);
}

bool CursorInjection::GetCursorVisibility() {
  return delegate_->CallFunctionName("cursorVisibility") == "true";
}

std::string CursorInjection::GetCursorState() {
  return "{ \"visibility\" : " +
         delegate_->CallFunctionName("cursorVisibility") + " }";
}

bool CursorInjection::SetCursor(gin::Arguments* args) {
  return delegate_->SetCursor(args);
}

class WindowInjection : public gin::Wrappable<WindowInjection> {
 public:
  static gin::WrapperInfo kWrapperInfo;

  class Delegate {
   public:
    virtual void SetInputRegion(gin::Arguments* args) = 0;
    virtual void SetWindowProperty(gin::Arguments* args) = 0;
  };

  WindowInjection(Delegate* delegate);
  ~WindowInjection() = default;

 private:
  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  void SetInputRegion(gin::Arguments* args);
  void SetProperty(gin::Arguments* args);

  Delegate* delegate_;
};

WindowInjection::WindowInjection(Delegate* delegate) : delegate_(delegate) {}

gin::WrapperInfo WindowInjection::kWrapperInfo = {gin::kEmbedderNativeGin};
gin::ObjectTemplateBuilder WindowInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WindowInjection>::GetObjectTemplateBuilder(isolate)
      .SetMethod("setInputRegion", &WindowInjection::SetInputRegion)
      .SetMethod("setProperty", &WindowInjection::SetProperty);
}

void WindowInjection::SetInputRegion(gin::Arguments* args) {
  delegate_->SetInputRegion(args);
}

void WindowInjection::SetProperty(gin::Arguments* args) {
  delegate_->SetWindowProperty(args);
}

class WebOSSystemInjection : public gin::Wrappable<WebOSSystemInjection>,
                             public InjectionWebOS,
                             public WindowInjection::Delegate,
                             public CursorInjection::Delegate {
 public:
  static gin::WrapperInfo kWrapperInfo;

  WebOSSystemInjection();
  ~WebOSSystemInjection() override;

  void BuildExtraObjects(v8::Local<v8::Object> obj,
                         v8::Isolate* isolate,
                         v8::Local<v8::Context> context);

  // Override WindowInjection::Delegate
  void SetInputRegion(gin::Arguments* args) override;
  void SetWindowProperty(gin::Arguments* args) override;
  // Override CursorInjection::Delegate
  std::string CallFunctionName(const char* name) override;
  bool SetCursor(gin::Arguments* args) override;

 private:
  // gin::Wrappable.
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) final;

  // Handlers for JS properties.
  std::string GetResource(gin::Arguments* args);
  double DevicePixelRatio();
  std::string GetCountry();
  bool GetIsMinimal();
  std::string GetActivityId();
  std::string GetDeviceInfo();
  std::string GetTimeZone();
  std::string GetTimeFormat();
  std::string GetIdentifier();
  std::string GetLocale();
  std::string GetLocaleRegion();
  std::string GetPhoneRegion();
  std::string GetScreenOrientation();
  bool GetIsActivated();
  std::string GetLaunchParams();
  void SetLaunchParams(gin::Arguments* args);
  std::string GetWindowOrientation();
  void SetWindowOrientation(gin::Arguments* args);
  bool GetIsKeyboardVisible();
  void Activate();
  void Deactivate();
  void OnCloseNotify(gin::Arguments* args);
  void NativePmLogInfoWithClock(gin::Arguments* args);
  void NativePmLogString(gin::Arguments* args);
  void NativePmTrace(gin::Arguments* args);
  void NativePmTraceItem(gin::Arguments* args);
  void NativePmTraceBefore(gin::Arguments* args);
  void NativePmTraceAfter(gin::Arguments* args);
  void AddBannerMessage(gin::Arguments* args);
  void RemoveBannerMessage(gin::Arguments* args);
  void ClearBannerMessages();
  void SimulateMouseClick(gin::Arguments* args);
  void UseSimulatedMouseClicks(gin::Arguments* args);
  void Paste();
  void CopiedToClipboard();
  void PastedFromClipboard();
  void MarkFirstUseDone();
  void EnableFullScreenMode(gin::Arguments* args);
  void StagePreparing(gin::Arguments* args);
  void StageReady(gin::Arguments* args);
  void ContainerReady(gin::Arguments* args);
  void EditorFocused();
  void KeepAlive(gin::Arguments* args);
  void ApplyLaunchFeedback();
  std::string AddNewContentIndicator();
  void RemoveNewContentIndicator(gin::Arguments* args);
  void KeyboardShow(gin::Arguments* args);
  void KeyboardHide();
  void SetManualKeyboardEnabled(gin::Arguments* args);
  void PlatformBack();
  void SetKeyMask(gin::Arguments* args);
  void SetLoadErrorPolicy(gin::Arguments* args);
  void Hide();
  void FocusOwner();
  void FocusLayer();
  std::string ServiceCall(gin::Arguments* args);
  void SetAppInClosing();
  bool DidRunOnCloseCallback();
  void UpdateInjectionData(gin::Arguments* args);
  void ReloadInjectionData();

  // webOSServiceBridge
  void InstallWebOSServiceBridge(v8::Isolate* isolate,
                                 v8::Local<v8::Context> context);
  static void WebOSServiceBridgeConstructorCallback(gin::Arguments* args);
  static v8::Local<v8::ObjectTemplate> MakeRequestTemplate(
      v8::Isolate* isolate);
  static v8::Persistent<v8::ObjectTemplate> request_template_;

  std::string GetInjectionData(const std::string& name);

  WebOSSystemDataManager* data_manager_;

  DISALLOW_COPY_AND_ASSIGN(WebOSSystemInjection);
};

gin::WrapperInfo WebOSSystemInjection::kWrapperInfo = {gin::kEmbedderNativeGin};
v8::Persistent<v8::ObjectTemplate> WebOSSystemInjection::request_template_;

WebOSSystemInjection::WebOSSystemInjection() {
  data_manager_ = new WebOSSystemDataManager(CallFunction("initialize"));
}

WebOSSystemInjection::~WebOSSystemInjection() {
  if (data_manager_)
    delete data_manager_;
}

void WebOSSystemInjection::BuildExtraObjects(v8::Local<v8::Object> obj,
                                             v8::Isolate* isolate,
                                             v8::Local<v8::Context> context) {
  const char kExtraObjectsForWebOSSystemInjectionAPI[] =
      "var webOSGetResource;"
      "if (typeof(webOSGetResource) == 'undefined') {"
      "  webOSGetResource = {};"
      "};"
      "webOSGetResource = function(p1, p2) {"
      "  return webOSSystem.getResource(p1, p2);"
      "};"

      "var webos;"
      "if (typeof(webos) == 'undefined') {"
      "  webos = {};"
      "};"
      "webos = {"
      "  get timezone() {"
      "    return webOSSystem.timeZone;"
      "  },"
      "};"

      "webOSSystem.window.setFocus = function(arg) {"
      "   var value = 'false';"
      "   if(arg == true) value = 'true';"
      "   webOSSystem.window.setProperty('needFocus', value);"
      "};"

      "webOSSystem._onCloseWithNotify_ = function(arg) {"
      "  if (typeof(webOSSystem._onCloseCallback_) != 'undefined') {"
      "    if (typeof(webOSSystem._onCloseCallback_) == 'function') {"
      "      webOSSystem._setAppInClosing_();"
      "      webOSSystem._onCloseCallback_(arg);"
      "      if (webOSSystem._didRunOnCloseCallback_() == true) {"
      "        webOSSystem.onCloseNotify(\"didRunOnCloseCallback\");"
      "      }"
      "    };"
      "  } else {"
      "    console.log('Callback is undefined');"
      "  };"
      "};"

      "webOSSystem.close = function(p1) {"
      "  if (p1 && p1 == 'EXIT_TO_LASTAPP') {"
      "    "
      "webOSSystem.window.setProperty('_WEBOS_LAUNCH_PREV_APP_AFTER_CLOSING', "
      "'true');"
      "  }"
      "  if (self !== top)"
      "    top.window.close();"
      "  else"
      "    window.close();"
      "};"

      "Object.defineProperty(webOSSystem, \"onclose\", {"
      "  set: function(p1) {"
      "    if (typeof(p1) == 'function') {"
      "      webOSSystem._onCloseCallback_ = p1;"
      "      webOSSystem.onCloseNotify(\"didSetOnCloseCallback\");"
      "    } else if(typeof(p1) === 'undefined' || p1 === undefined) {"
      "      webOSSystem._onCloseCallback_ = p1;"
      "      webOSSystem.onCloseNotify(\"didClearOnCloseCallback\");"
      "    } else {"
      "      console.log('Parameter is not a function');"
      "    };"
      "  },"
      "  get: function() {"
      "    return webOSSystem._onCloseCallback_;"
      "  }"
      "});"

      "Object.defineProperty(this, \"devicePixelRatio\", {"
      "  get: function() {"
      "    return webOSSystem.devicePixelRatio();"
      "  }"
      "});"

      // Place this code always at the end of injection API
      // Support PalmServiceBridge for backward compatibility
      "var PalmServiceBridge;"
      "if (typeof(PalmServiceBridge) == 'undefined') {"
      "  PalmServiceBridge = webOSServiceBridge;"
      "};"

      // Support PalmSystem for backward compatibility
      "var palmGetResource;"
      "if (typeof(palmGetResource) == 'undefined') {"
      "  palmGetResource = webOSGetResource"
      "};"

      "var PalmSystem;"
      "if (typeof(PalmSystem) == 'undefined') {"
      "  PalmSystem = webOSSystem;"
      "};";

  // Build webOSServiceBridge
  InstallWebOSServiceBridge(isolate, context);
  // Build webOSSystem.window
  gin::Handle<WindowInjection> window_obj =
      gin::CreateHandle(isolate, new WindowInjection(this));
  obj->Set(context, gin::StringToV8(isolate, "window"), window_obj.ToV8());
  // Build webOSSystem.cursor
  gin::Handle<CursorInjection> cursor_obj =
      gin::CreateHandle(isolate, new CursorInjection(this));
  obj->Set(context, gin::StringToV8(isolate, "cursor"), cursor_obj.ToV8());
  v8::Script::CompileExtraInjections(
      isolate,
      gin::StringToV8(isolate, kExtraObjectsForWebOSSystemInjectionAPI),
      WebOSSystemInjectionExtension::kWebOSSystemInjectionName);
}

gin::ObjectTemplateBuilder WebOSSystemInjection::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<WebOSSystemInjection>::GetObjectTemplateBuilder(isolate)
      .SetMethod("getResource", &WebOSSystemInjection::GetResource)
      .SetMethod("devicePixelRatio", &WebOSSystemInjection::DevicePixelRatio)
      .SetProperty("country", &WebOSSystemInjection::GetCountry)
      .SetProperty("isMinimal", &WebOSSystemInjection::GetIsMinimal)
      .SetProperty("activityId", &WebOSSystemInjection::GetActivityId)
      .SetProperty("deviceInfo", &WebOSSystemInjection::GetDeviceInfo)
      .SetProperty("timeZone", &WebOSSystemInjection::GetTimeZone)
      .SetProperty("timeFormat", &WebOSSystemInjection::GetTimeFormat)
      .SetProperty("identifier", &WebOSSystemInjection::GetIdentifier)
      .SetProperty("locale", &WebOSSystemInjection::GetLocale)
      .SetProperty("localeRegion", &WebOSSystemInjection::GetLocaleRegion)
      .SetProperty("phoneRegion", &WebOSSystemInjection::GetPhoneRegion)
      .SetProperty("screenOrientation",
                   &WebOSSystemInjection::GetScreenOrientation)
      .SetProperty("isActivated", &WebOSSystemInjection::GetIsActivated)
      .SetProperty("launchParams", &WebOSSystemInjection::GetLaunchParams,
                   &WebOSSystemInjection::SetLaunchParams)
      .SetProperty("windowOrientation",
                   &WebOSSystemInjection::GetWindowOrientation,
                   &WebOSSystemInjection::SetWindowOrientation)
      .SetProperty("isKeyboardVisible",
                   &WebOSSystemInjection::GetIsKeyboardVisible)
      .SetMethod("activate", &WebOSSystemInjection::Activate)
      .SetMethod("deactivate", &WebOSSystemInjection::Deactivate)
      .SetMethod("onCloseNotify", &WebOSSystemInjection::OnCloseNotify)
      .SetMethod("getIdentifier", &WebOSSystemInjection::GetIdentifier)
      .SetMethod("setWindowOrientation",
                 &WebOSSystemInjection::SetWindowOrientation)
      .SetMethod("PmLogInfoWithClock",
                 &WebOSSystemInjection::NativePmLogInfoWithClock)
      .SetMethod("PmLogString", &WebOSSystemInjection::NativePmLogString)
      .SetMethod("PmTrace", &WebOSSystemInjection::NativePmTrace)
      .SetMethod("PmTraceItem", &WebOSSystemInjection::NativePmTraceItem)
      .SetMethod("PmTraceBefore", &WebOSSystemInjection::NativePmTraceBefore)
      .SetMethod("PmTraceAfter", &WebOSSystemInjection::NativePmTraceAfter)
      .SetMethod("addBannerMessage", &WebOSSystemInjection::AddBannerMessage)
      .SetMethod("removeBannerMessage",
                 &WebOSSystemInjection::RemoveBannerMessage)
      .SetMethod("clearBannerMessages",
                 &WebOSSystemInjection::ClearBannerMessages)
      .SetMethod("simulateMouseClick",
                 &WebOSSystemInjection::SimulateMouseClick)
      .SetMethod("useSimulatedMouseClicks",
                 &WebOSSystemInjection::UseSimulatedMouseClicks)
      .SetMethod("paste", &WebOSSystemInjection::Paste)
      .SetMethod("copiedToClipboard", &WebOSSystemInjection::CopiedToClipboard)
      .SetMethod("pastedFromClipboard",
                 &WebOSSystemInjection::PastedFromClipboard)
      .SetMethod("markFirstUseDone", &WebOSSystemInjection::MarkFirstUseDone)
      .SetMethod("enableFullScreenMode",
                 &WebOSSystemInjection::EnableFullScreenMode)
      .SetMethod("stagePreparing", &WebOSSystemInjection::StagePreparing)
      .SetMethod("stageReady", &WebOSSystemInjection::StageReady)
      .SetMethod("containerReady", &WebOSSystemInjection::ContainerReady)
      .SetMethod("editorFocused", &WebOSSystemInjection::EditorFocused)
      .SetMethod("keepAlive", &WebOSSystemInjection::KeepAlive)
      .SetMethod("applyLaunchFeedback",
                 &WebOSSystemInjection::ApplyLaunchFeedback)
      .SetMethod("addNewContentIndicator",
                 &WebOSSystemInjection::AddNewContentIndicator)
      .SetMethod("removeNewContentIndicator",
                 &WebOSSystemInjection::RemoveNewContentIndicator)
      .SetMethod("keyboardShow", &WebOSSystemInjection::KeyboardShow)
      .SetMethod("keyboardHide", &WebOSSystemInjection::KeyboardHide)
      .SetMethod("setManualKeyboardEnabled",
                 &WebOSSystemInjection::SetManualKeyboardEnabled)
      .SetMethod("platformBack", &WebOSSystemInjection::PlatformBack)
      .SetMethod("setKeyMask", &WebOSSystemInjection::SetKeyMask)
      .SetMethod("setLoadErrorPolicy",
                 &WebOSSystemInjection::SetLoadErrorPolicy)
      .SetMethod("hide", &WebOSSystemInjection::Hide)
      .SetMethod("focusOwner", &WebOSSystemInjection::FocusOwner)
      .SetMethod("focusLayer", &WebOSSystemInjection::FocusLayer)
      .SetMethod("setInputRegion", &WebOSSystemInjection::SetInputRegion)
      .SetMethod("setWindowProperty", &WebOSSystemInjection::SetWindowProperty)
      .SetMethod("setCursor", &WebOSSystemInjection::SetCursor)
      .SetMethod("_setAppInClosing_", &WebOSSystemInjection::SetAppInClosing)
      .SetMethod("_didRunOnCloseCallback_",
                 &WebOSSystemInjection::DidRunOnCloseCallback)
      .SetMethod("updateInjectionData",
                 &WebOSSystemInjection::UpdateInjectionData)
      .SetMethod("reloadInjectionData",
                 &WebOSSystemInjection::ReloadInjectionData)
      .SetMethod("serviceCall", &WebOSSystemInjection::ServiceCall);
}

void WebOSSystemInjection::ReloadInjectionData() {
  data_manager_->SetInitializedStatus(false);
  data_manager_->DoInitialize(CallFunction("initialize"));
}

void WebOSSystemInjection::UpdateInjectionData(gin::Arguments* args) {
  std::string key;
  std::string value;
  if (args->Length() < 2 || !args->GetNext(&key) || !args->GetNext(&value))
    return;

  data_manager_->UpdateInjectionData(key, value);
}

bool WebOSSystemInjection::DidRunOnCloseCallback() {
  return WebOSServiceBridgeInjection::waiting_responses_.empty();
}

void WebOSSystemInjection::SetAppInClosing() {
  // Even if app A set this class static variable to true
  // This does not affect to other apps
  WebOSServiceBridgeInjection::is_closing_ = true;
}

bool WebOSSystemInjection::SetCursor(gin::Arguments* args) {
  std::string cursor_arg;
  if (!args->Length() || !args->GetNext(&cursor_arg))
    return false;

  std::string x = "0";
  std::string y = "0";

  int arg2;
  if (args->Length() >= 2 && args->GetNext(&arg2))
    x = std::to_string(arg2);
  int arg3;
  if (args->Length() >= 3 && args->GetNext(&arg3))
    y = std::to_string(arg3);

  if (cursor_arg.empty()) {
    cursor_arg = "default";
  } else if (cursor_arg != "default" && cursor_arg != "blank") {
    // custom cursor : need to check file path
    cursor_arg =
        checkFileValidation(cursor_arg, GetInjectionData("folderPath"));
    if (cursor_arg.empty())
      return false;
  }

  std::vector<std::string> arguments;
  arguments.push_back(cursor_arg);
  arguments.push_back(x);
  arguments.push_back(y);
  SendCommand("setCursor", arguments);

  return true;
}

std::string WebOSSystemInjection::CallFunctionName(const char* name) {
  return CallFunction(name);
}

void WebOSSystemInjection::SetWindowProperty(gin::Arguments* args) {
  if (args->Length() <= 0)
    return;

  std::string arg1;
  std::string arg2;

  if (!args->GetNext(&arg1) || !args->GetNext(&arg2))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(arg1);
  arguments.push_back(arg2);
  SendCommand("setWindowProperty", arguments);
}

void WebOSSystemInjection::SetInputRegion(gin::Arguments* args) {
  if (args->Length() <= 0)
    return;

  v8::Isolate* isolate = args->isolate();
  v8::Local<v8::Context> context = isolate->GetCurrentContext();

  v8::Local<v8::Value> json_string[] = {args->PeekNext()};
  gin::Dictionary window_object(isolate, context->Global());
  v8::Local<v8::Object> json_object;
  window_object.Get(std::string("JSON"), &json_object);
  gin::Dictionary json_dic(isolate, json_object);
  v8::Local<v8::Function> stringify_func;
  json_dic.Get(std::string("stringify"), &stringify_func);

  std::vector<std::string> arguments;
  arguments.push_back(*v8::String::Utf8Value(
      stringify_func->Call(json_object, 1, json_string)));
  SendCommand("setInputRegion", arguments);
}

std::string WebOSSystemInjection::ServiceCall(gin::Arguments* args) {
  if (args->Length() < 2)
    return std::string("false");

  v8::Isolate* isolate = args->isolate();
  std::string url;
  std::string payload;

  if (!args->GetNext(&url) || !args->GetNext(&payload))
    return std::string("false");

  // check trustLevel : only trusted webapp can call this function
  if (GetInjectionData("trustLevel") != "trusted") {
    return std::string("false");
  }

  // This member is static class variable
  // But this value is separated between apps
  if (!WebOSServiceBridgeInjection::is_closing_) {
    return std::string("false");
  }
  if (url.empty() || payload.empty()) {
    return std::string("false");
  }

  std::vector<std::string> arguments;
  arguments.push_back(url);
  arguments.push_back(payload);
  SendCommand("serviceCall", arguments);
  return std::string("true");
}

void WebOSSystemInjection::FocusLayer() {
  SendCommand("focusLayer");
}

void WebOSSystemInjection::FocusOwner() {
  SendCommand("focusOwner");
}

void WebOSSystemInjection::Hide() {
  SendCommand("hide");
}

void WebOSSystemInjection::SetLoadErrorPolicy(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(param);
  SendCommand("setLoadErrorPolicy", arguments);
}

void WebOSSystemInjection::SetKeyMask(gin::Arguments* args) {
  if (!args->Length())
    return;

  std::string output_js;
  std::vector<std::string> arguments;
  base::ListValue* root = new base::ListValue();

  v8::Local<v8::Array> array = v8::Local<v8::Array>::Cast(args->PeekNext());

  for (int i = 0; i < array->Length(); i++) {
    v8::Local<v8::Value> item = array->Get(i);
    root->AppendString(*v8::String::Utf8Value(item->ToString()));
  }
  base::JSONWriter::Write(*root, &output_js);

  arguments.push_back(output_js);
  SendCommand("setKeyMask", arguments);
}

void WebOSSystemInjection::PlatformBack() {
  SendCommand("platformBack");
}

void WebOSSystemInjection::SetManualKeyboardEnabled(gin::Arguments* args) {
  bool param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  if (param)
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("setManualKeyboardEnabled", arguments);
}

void WebOSSystemInjection::KeyboardHide() {
  SendCommand("keyboardHide");
}

void WebOSSystemInjection::KeyboardShow(gin::Arguments* args) {
  int param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(std::to_string(param));
  SendCommand("keyboardShow", arguments);
}

void WebOSSystemInjection::RemoveNewContentIndicator(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(param);
  SendCommand("removeNewContentIndicator", arguments);
}

std::string WebOSSystemInjection::AddNewContentIndicator() {
  return CallFunction("addNewContentIndicator");
}

void WebOSSystemInjection::ApplyLaunchFeedback() {
  // NOTIMPLEMENTED();
}

void WebOSSystemInjection::KeepAlive(gin::Arguments* args) {
  bool param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  if (param)
    arguments.push_back("true");
  else
    arguments.push_back("false");

  // Set websetting first because it is same renderer
  setKeepAliveWebApp(param);
  SendCommand("keepAlive", arguments);
}

void WebOSSystemInjection::EditorFocused() {
  // NOTIMPLEMENTED();
}

void WebOSSystemInjection::ContainerReady(gin::Arguments* args) {
  SendCommand("containerReady");
}

void WebOSSystemInjection::StageReady(gin::Arguments* args) {
  SendCommand("stageReady");
}

void WebOSSystemInjection::StagePreparing(gin::Arguments* args) {
  SendCommand("stagePreparing");
}

void WebOSSystemInjection::EnableFullScreenMode(gin::Arguments* args) {
  bool param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  if (param)
    arguments.push_back("true");
  else
    arguments.push_back("false");

  SendCommand("enableFullScreenMode", arguments);
}

void WebOSSystemInjection::MarkFirstUseDone() {
  // NOTIMPLEMENTED();
}

void WebOSSystemInjection::PastedFromClipboard() {
  // NOTIMPLEMENTED();
}

void WebOSSystemInjection::CopiedToClipboard() {
  // NOTIMPLEMENTED();
}

void WebOSSystemInjection::Paste() {
  SendCommand("paste");
}

void WebOSSystemInjection::UseSimulatedMouseClicks(gin::Arguments* args) {
  std::vector<std::string> arguments;
  if (args->PeekNext()->IsTrue())
    arguments.push_back("true");
  else
    arguments.push_back("false");
  SendCommand("useSimulatedMouseClicks", arguments);
}

void WebOSSystemInjection::SimulateMouseClick(gin::Arguments* args) {
  if (args->Length() < 3)
    return;

  std::string result0;
  std::string result1;
  bool result2;

  if (!args->GetNext(&result0) || !args->GetNext(&result1) ||
      !args->GetNext(&result2))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(result0);
  arguments.push_back(result1);

  if (result2)
    arguments.push_back("true");
  else
    arguments.push_back("false");

  SendCommand("simulateMouseClick", arguments);
}

void WebOSSystemInjection::ClearBannerMessages() {
  SendCommand("clearBannerMessages");
}

void WebOSSystemInjection::RemoveBannerMessage(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(param);
  SendCommand("removeBannerMessage", arguments);
}

void WebOSSystemInjection::AddBannerMessage(gin::Arguments* args) {
  if (args->Length() < 5)
    return;

  std::string param;
  std::vector<std::string> arguments;

  while (args->GetNext(&param)) {
    arguments.push_back(param);
  }
  args->Return(CallFunction("addBannerMessage", arguments));
}

void WebOSSystemInjection::NativePmTraceAfter(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;
  PMTRACE_AFTER(const_cast<char*>(param.c_str()));
}

void WebOSSystemInjection::NativePmTraceBefore(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;
  PMTRACE_BEFORE(const_cast<char*>(param.c_str()));
}

void WebOSSystemInjection::NativePmTraceItem(gin::Arguments* args) {
  if (args->Length() < 2)
    return;

  std::string param0;
  std::string param1;

  if (!args->GetNext(&param0) || !args->GetNext(&param1))
    return;

  PMTRACE_ITEM(const_cast<char*>(param0.c_str()),
               const_cast<char*>(param1.c_str()));
}

void WebOSSystemInjection::NativePmTrace(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;
  PMTRACE_LOG(const_cast<char*>(param.c_str()));
}

void WebOSSystemInjection::NativePmLogString(gin::Arguments* args) {
  if (args->Length() < 4)
    return;

  v8::Local<v8::Value> param;
  std::vector<std::string> arguments;

  while (args->GetNext(&param)) {
    arguments.push_back(*v8::String::Utf8Value(param));
  }
  SendCommand("PmLogString", arguments);
}

void WebOSSystemInjection::NativePmLogInfoWithClock(gin::Arguments* args) {
  if (args->Length() < 3)
    return;

  std::string param;
  std::vector<std::string> arguments;

  while (args->GetNext(&param)) {
    arguments.push_back(param);
  }
  SendCommand("PmLogInfoWithClock", arguments);
}

void WebOSSystemInjection::OnCloseNotify(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  PMLOG_INFO(Injection, "[webOSSystem]", "webOSSystem.OnCloseNotify(%s)",
             param.c_str());
  std::vector<std::string> arguments;
  arguments.push_back(param);
  SendCommand("onCloseNotify", arguments);
}

void WebOSSystemInjection::Deactivate() {
  SendCommand("deactivate");
}

void WebOSSystemInjection::Activate() {
  SendCommand("activate");
}

bool WebOSSystemInjection::GetIsKeyboardVisible() {
  return CallFunction("isKeyboardVisible") == "true" ? true : false;
}

std::string WebOSSystemInjection::GetWindowOrientation() {
  return GetInjectionData("windowOrientation");
}

void WebOSSystemInjection::SetWindowOrientation(gin::Arguments* args) {
  std::string param;
  if (!args->Length() || !args->GetNext(&param))
    return;

  std::vector<std::string> arguments;
  arguments.push_back(param);
  SendCommand("setWindowOrientation", arguments);
}

std::string WebOSSystemInjection::GetLaunchParams() {
  return GetInjectionData("launchParams");
}

void WebOSSystemInjection::SetLaunchParams(gin::Arguments* args) {
  std::string launchParams;

  if (!args->Length() || !args->GetNext(&launchParams))
    return;

  std::vector<std::string> arguments;

  if (data_manager_)
    data_manager_->UpdateInjectionData("launchParams", launchParams);

  arguments.push_back(launchParams);
  SendCommand("launchParams", arguments);
}

bool WebOSSystemInjection::GetIsActivated() {
  return GetInjectionData("isActivated") == "true";
}

std::string WebOSSystemInjection::GetScreenOrientation() {
  return GetInjectionData("screenOrientation");
}

std::string WebOSSystemInjection::GetPhoneRegion() {
  return GetInjectionData("phoneRegion");
}

std::string WebOSSystemInjection::GetLocaleRegion() {
  return GetInjectionData("localeRegion");
}

std::string WebOSSystemInjection::GetLocale() {
  return GetInjectionData("locale");
}

std::string WebOSSystemInjection::GetIdentifier() {
  return GetInjectionData("identifier");
}

std::string WebOSSystemInjection::GetTimeFormat() {
  return GetInjectionData("timeFormat");
}

std::string WebOSSystemInjection::GetTimeZone() {
  time_t localTime = time(0);
  tm localTM;
  localTM = *localtime(&localTime);

  return localTM.tm_zone ? localTM.tm_zone : "";
}

std::string WebOSSystemInjection::GetDeviceInfo() {
  return GetInjectionData("deviceInfo");
}

std::string WebOSSystemInjection::GetActivityId() {
  return GetInjectionData("activityId");
}

bool WebOSSystemInjection::GetIsMinimal() {
  return GetInjectionData("isMinimal") == "true" ? true : false;
}

std::string WebOSSystemInjection::GetCountry() {
  return GetInjectionData("country");
}

std::string WebOSSystemInjection::GetInjectionData(const std::string& name) {
  if (!data_manager_->GetInitializedStatus()) {
    data_manager_->DoInitialize(CallFunction("initialize"));
  }
  if (data_manager_->GetInitializedStatus()) {
    std::string result;
    if (data_manager_->GetInjectionData(name, result))
      return result;
  }
  return CallFunction(name);
}

std::string WebOSSystemInjection::GetResource(gin::Arguments* args) {
  v8::Isolate* isolate = args->isolate();
  std::string arg1;
  if (args->PeekNext().IsEmpty() || !args->GetNext(&arg1)) {
    return std::string();
  }
  std::vector<std::string> arguments;
  arguments.push_back(arg1);
  return CallFunction("getResource", arguments);
}

double WebOSSystemInjection::DevicePixelRatio() {
  return std::stod(GetInjectionData("devicePixelRatio"));
}

void WebOSSystemInjection::InstallWebOSServiceBridge(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context) {
  v8::HandleScope handle_scope(isolate);

  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();

  v8::Local<v8::FunctionTemplate> templ =
      gin::CreateFunctionTemplateForConstructorBehavior(
          isolate,
          base::Bind(
              &WebOSSystemInjection::WebOSServiceBridgeConstructorCallback));

  global->Set(gin::StringToSymbol(isolate, "webOSServiceBridge"),
              templ->GetFunction());
}

// static
v8::Local<v8::ObjectTemplate> WebOSSystemInjection::MakeRequestTemplate(
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
void WebOSSystemInjection::WebOSServiceBridgeConstructorCallback(
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

void WebOSSystemInjectionExtension::Install(blink::WebLocalFrame* frame) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context = frame->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);
  v8::Local<v8::Object> global = context->Global();
  v8::Local<v8::Value> webossystem_value =
      global->Get(gin::StringToV8(isolate, "webOSSystem"));
  if (!webossystem_value.IsEmpty() && webossystem_value->IsObject())
    return;

  v8::Local<v8::Object> webossystem =
      CreateWebOSSystemObject(isolate, context, global);
  WebOSSystemInjection* webossystem_injection = nullptr;
  gin::Converter<WebOSSystemInjection*>::FromV8(isolate, webossystem,
                                                &webossystem_injection);
  webossystem_injection->BuildExtraObjects(webossystem, isolate, context);
}

v8::Local<v8::Object> WebOSSystemInjectionExtension::CreateWebOSSystemObject(
    v8::Isolate* isolate,
    v8::Local<v8::Context> context,
    v8::Local<v8::Object> global) {
  gin::Handle<WebOSSystemInjection> webossystem =
      gin::CreateHandle(isolate, new WebOSSystemInjection());
  global->Set(context, gin::StringToV8(isolate, "webOSSystem"),
              webossystem.ToV8());
  return webossystem->GetWrapper(isolate).ToLocalChecked();
}

}  // namespace extensions_v8
