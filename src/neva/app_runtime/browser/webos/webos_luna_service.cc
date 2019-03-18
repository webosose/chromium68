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

#include "neva/app_runtime/browser/webos/webos_luna_service.h"

#include <glib.h>

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/logging_pmlog.h"

#define DEBUG_LOG(format, ...) \
  PMLOG_DEBUG(WebOSLunaService, format, ##__VA_ARGS__)

namespace neva {

enum LAUNCH_TARGET {
  NETWORK = 1,
  GENERAL,
};

const char kGetSystemSettings[] =
    "luna://com.webos.settingsservice/getSystemSettings";
const char kLaunchApplication[] = "palm://com.webos.applicationManager/launch";
const char kServiceName[] = "com.webos.settingsservice.client";

// Keys and Parameters
const char kSubscribe[] = "subscribe";
const char kParams[] = "params";
const char kTarget[] = "target";

struct AutoLSError : LSError {
  AutoLSError() { LSErrorInit(this); }
  ~AutoLSError() { LSErrorFree(this); }
};

std::string MapTargetIdToString(int target_id) {
  std::string ret;
  switch (target_id) {
    case NETWORK:
      ret = "network";
      break;
    case GENERAL:
      ret = "general";
      break;
  }
  return ret;
}

WebOSLunaService::WebOSLunaService()
    : handle_(nullptr), context_(nullptr), initialized_(false) {
  AutoLSError error;

  if (!LSRegister(kServiceName, &handle_, &error)) {
    DEBUG_LOG("LSRegister failed : %s", error.message);
    return;
  }

  // Make sure that use appropriate g_main_loop in WebAppManagerService
  context_ = g_main_context_ref(g_main_context_default());
  if (!LSGmainContextAttach(handle_, g_main_context_default(), &error)) {
    DEBUG_LOG("LSGmainContextAttach failed : %s", error.message);
    return;
  }
}

WebOSLunaService::~WebOSLunaService() {
  AutoLSError error;

  if (!LSUnregister(handle_, &error))
    DEBUG_LOG("LSUnregister failed : %s", error.message);

  g_main_context_unref(context_);
}

WebOSLunaService* WebOSLunaService::GetInstance() {
  return base::Singleton<WebOSLunaService>::get();
}

bool WebOSLunaService::LunaServiceCall(const std::string& uri,
                                       const base::Value* parameters,
                                       LSFilterFunc callback,
                                       void* context) {
  if (!handle_ || !parameters)
    return false;

  bool subscribe = false;
  if (parameters->is_dict()) {
    const base::DictionaryValue* dict =
        static_cast<const base::DictionaryValue*>(parameters);
    dict->GetBoolean(kSubscribe, &subscribe);
  }

  std::string payload;
  base::JSONWriter::Write(*parameters, &payload);
  DEBUG_LOG("uri[%s] payload[%s]", uri.c_str(), payload.c_str());

  AutoLSError error;
  if (subscribe) {
    LSMessageToken token;
    if (!LSCall(handle_, uri.c_str(), payload.c_str(), callback, context,
                &token, &error)) {
      DEBUG_LOG("LSCall failed : %s", error.message);
      return false;
    }
    ls_token_vector_.push_back(token);
    return true;
  }

  if (!LSCallOneReply(handle_, uri.c_str(), payload.c_str(), callback, context,
                      nullptr, &error)) {
    DEBUG_LOG("LSCallOneReply failed : %s", error.message);
    return false;
  }

  return true;
}

void WebOSLunaService::LunaServiceCancel(LSMessageToken* token) {
  if (!handle_ || !token || *token == LSMESSAGE_TOKEN_INVALID)
    return;

  AutoLSError error;
  if (!LSCallCancel(handle_, *token, &error)) {
    DEBUG_LOG("LSCallCancle failed : %s", error.message);
    return;
  }

  *token = LSMESSAGE_TOKEN_INVALID;
}

void WebOSLunaService::Initialize() {
  if (initialized_)
    return;

  // Get system settings
  GetSystemSettings();

  initialized_ = true;
}

void WebOSLunaService::GetSystemSettings() {
  const std::unique_ptr<base::DictionaryValue> resource(
      new base::DictionaryValue());

  resource->SetString("key", "localeInfo");
  resource->SetBoolean(kSubscribe, true);

  if (!LunaServiceCall(kGetSystemSettings, resource.get(),
                       &WebOSLunaService::GetSystemSettingsCb, this)) {
    DEBUG_LOG("GetSystemSettings LSCall failed");
  }
}

void WebOSLunaService::Finalize() {
  if (!initialized_)
    return;
  DEBUG_LOG("WebOSLunaService::Finalize");

  LSMessageToken token;
  for (std::vector<LSMessageToken>::iterator it = ls_token_vector_.begin();
       it != ls_token_vector_.end(); ++it) {
    token = *it;
    LunaServiceCancel(&token);
  }

  if (ls_token_vector_.size() > 0)
    ls_token_vector_.clear();

  initialized_ = false;
}

void WebOSLunaService::LaunchSettingsApplication(int target_id) {
  std::string target_string = MapTargetIdToString(target_id);
  if (target_string.empty())
    return;

  const std::unique_ptr<base::DictionaryValue> resource(
      new base::DictionaryValue());
  resource->SetString("id", "com.palm.app.settings");

  const std::unique_ptr<base::DictionaryValue> params(
      new base::DictionaryValue());
  const std::unique_ptr<base::DictionaryValue> target(
      new base::DictionaryValue());
  target->SetString(kTarget, target_string);
  resource->Set(kParams, base::Value::ToUniquePtrValue(target->Clone()));

  if (!LunaServiceCall(kLaunchApplication, resource.get(),
                       &WebOSLunaService::LaunchApplicationStatusCb, this)) {
    DEBUG_LOG("GetSystemSettings LSCall failed");
  }
}

void WebOSLunaService::NotifySystemLocale(const std::string& ui) {
  if (delegate_)
    delegate_->NotifySystemLocale(ui);
}

bool WebOSLunaService::GetSystemSettingsCb(LSHandle* handle,
                                           LSMessage* reply,
                                           void* context) {
  WebOSLunaService* luna_service = static_cast<WebOSLunaService*>(context);

  std::string payload = const_cast<char*>(LSMessageGetPayload(reply));
  std::unique_ptr<base::Value> value(base::JSONReader().ReadToValue(payload));
  const base::DictionaryValue* resourceponse = nullptr;
  const base::DictionaryValue* settings = nullptr;

  if (!value->GetAsDictionary(&resourceponse))
    return false;

  if (!resourceponse->GetDictionary("settings", &settings))
    return false;

  const base::DictionaryValue* localeInfo = nullptr;
  if (!settings->GetDictionary("localeInfo", &localeInfo))
    return false;

  const base::DictionaryValue* locales = nullptr;
  if (!localeInfo->GetDictionary("locales", &locales))
    return false;

  std::string ui;
  if (locales->GetString("UI", &ui)) {
    luna_service->NotifySystemLocale(ui);
    return true;
  }

  return false;
}

bool WebOSLunaService::LaunchApplicationStatusCb(LSHandle* handle,
                                                 LSMessage* reply,
                                                 void* context) {
  NOTIMPLEMENTED();
  return false;
}

}  // namespace neva
