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

#include "media/base/neva/webos/lunaservice_client.h"

#include <glib.h>

#include "base/callback_helpers.h"
#include "base/logging.h"
#include "base/rand_util.h"

namespace media {

// Needs to match the definition of URIType in the header
static const char* const luna_service_uris[] = {
    "",                                            // VSM
    "",                                            // DISPLAY
    "",                                            // AVBLOCK
    "luna://com.webos.audio",                      // AUDIO
    "",                                            // BROADCAST
    "",                                            // CHANNEL
    "",                                            // EXTERNALDEVICE
    "",                                            // DVR
    "",                                            // SOUND
    "",                                            // SUBTITLE
    "",                                            // DRM
    "luna://com.webos.settingsservice",            // SETTING
    "",                                            // PHOTORENDERER
};

struct AutoLSError : LSError {
  AutoLSError() { LSErrorInit(this); }
  ~AutoLSError() { LSErrorFree(this); }
};

static const char kServiceName[] = "com.webos.chromium.media.";

// LunaServiceClient implematation
LunaServiceClient::LunaServiceClient(BusType type, bool need_service_name)
    : handle(NULL), context(NULL) {
  AutoLSError error;
  std::string service_name;
  if (need_service_name)
    service_name = kServiceName + std::to_string(base::RandInt(10000,99999));
  if (LSRegisterPubPriv(service_name.c_str(), &handle, type, &error)) {
    context = g_main_context_ref(g_main_context_default());
    LSGmainContextAttach(handle, context, &error);
  }
}

LunaServiceClient::~LunaServiceClient() {
  AutoLSError error;
  LSUnregister(handle, &error);
  g_main_context_unref(context);
}

bool handleAsync(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  LOG(INFO) << "[RES] - " << wrapper->uri << " " << dump;
  if (!wrapper->callback.is_null())
    base::ResetAndReturn(&wrapper->callback).Run(dump);

  LSMessageUnref(reply);

  delete wrapper;

  return true;
}

bool handleSubscribe(LSHandle* sh, LSMessage* reply, void* ctx) {
  LunaServiceClient::ResponseHandlerWrapper* wrapper =
      static_cast<LunaServiceClient::ResponseHandlerWrapper*>(ctx);

  LSMessageRef(reply);
  std::string dump = LSMessageGetPayload(reply);
  LOG(INFO) << "[SUB-RES] - " << wrapper->uri << " " << dump;
  if (!wrapper->callback.is_null())
    wrapper->callback.Run(dump);

  LSMessageUnref(reply);

  return true;
}

bool LunaServiceClient::callASync(const std::string& uri,
                                  const std::string& param) {
  ResponseCB nullcb;
  return callASync(uri, param, nullcb);
}

bool LunaServiceClient::callASync(const std::string& uri,
                                  const std::string& param,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  if (!wrapper)
    return false;

  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  LOG(INFO) << "[REQ] - " << uri << " " << param;
  if (!LSCallOneReply(handle, uri.c_str(), param.c_str(), handleAsync, wrapper,
                      NULL, &error)) {
    base::ResetAndReturn(&wrapper->callback).Run("");
    delete wrapper;
    return false;
  }

  return true;
}

bool LunaServiceClient::subscribe(const std::string& uri,
                                  const std::string& param,
                                  LSMessageToken* subscribeKey,
                                  const ResponseCB& callback) {
  AutoLSError error;
  ResponseHandlerWrapper* wrapper = new ResponseHandlerWrapper;
  if (!wrapper)
    return false;

  wrapper->callback = callback;
  wrapper->uri = uri;
  wrapper->param = param;

  if (!LSCall(handle, uri.c_str(), param.c_str(), handleSubscribe, wrapper,
              subscribeKey, &error)) {
    LOG(INFO) << "[SUB] " << uri << ":[" << param << "] fail["
              << error.message << "]";
    delete wrapper;
    return false;
  }

  handlers[*subscribeKey] = std::unique_ptr<ResponseHandlerWrapper>(wrapper);

  return true;
}

bool LunaServiceClient::unsubscribe(LSMessageToken subscribeKey) {
  AutoLSError error;

  if (!LSCallCancel(handle, subscribeKey, &error)) {
    LOG(INFO) << "[UNSUB] " << subscribeKey << " fail[" << error.message << "]";
    handlers.erase(subscribeKey);
    return false;
  }

  if (handlers[subscribeKey])
    handlers[subscribeKey]->callback.Reset();

  handlers.erase(subscribeKey);

  return true;
}

// static
std::string LunaServiceClient::GetServiceURI(URIType type,
                                             const std::string& action) {
  if (type < 0 || type > LunaServiceClient::URITypeMax)
    return std::string();

  std::string uri = luna_service_uris[type];
  uri.append("/");
  uri.append(action);
  return uri;
}

}  // namespace media
