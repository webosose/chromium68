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

#include <glib.h>
#include <lunaservice.h>

#include "base/logging.h"
#include "neva/webos_impl/luna/luna_client.h"

namespace {

const char kSignalUri[] = "palm://com.palm.bus/signal/addmatch";

struct LSErrorSafe: public ::LSError {
  LSErrorSafe() {
    LSErrorInit(this);
  };

  ~LSErrorSafe() {
    LSErrorFree(this);
  }
};

void logError(LSError& err) {
  LOG(ERROR) << "Luna Client error: "
    << err.error_code << ": " << err.message << " ("
    << err.func << " @ " << err.file << ":" << err.line << ")";
}

bool filterMessage(::LSHandle* handle, ::LSMessage* reply, void* ctx) {
  lunabus::Handler* handler = static_cast<lunabus::Handler*>(ctx);
  if (handler) {
    const char* payload = ::LSMessageGetPayload(reply);
    handler->Handle(payload);
    return true;
  }
  return false;
}

} // namespace

namespace lunabus {

Handler::~Handler() {
}

void Handler::SetToken(unsigned int token) {
  token_ = token;
}

unsigned int Handler::GetToken() const {
  return token_;
}

LunaClient::LunaClient(const char* id)
    : id_(id), initialized_(Initialize(id)) {
  DCHECK(id != nullptr);
}

LunaClient::~LunaClient() {
  if (!handle_)
    return;

  LSErrorSafe err;
  if (!::LSUnregister(handle_, &err))
    logError(err);
}

bool LunaClient::Initialized() const {
  return initialized_;
}

unsigned long LunaClient::Call(
    const char* uri, const char* payload, Handler* handler) {
  DCHECK(uri != nullptr);
  DCHECK(payload != nullptr);
  DCHECK(handler != nullptr);

  LSErrorSafe err;
  ::LSMessageToken token = 0;
  bool subscription = false;
  const bool result = (subscription)
      ? ::LSCall(handle_, uri, payload, filterMessage, handler, &token, &err)
      : ::LSCallOneReply(
            handle_, uri, payload, filterMessage, handler, &token, &err);

  if (result)
    handler->SetToken(token);
  else {
    logError(err);
    token = 0;
  }

  return token;
}

unsigned long LunaClient::Signal(const char* payload, Handler* handler) {
  DCHECK(payload != nullptr);
  DCHECK(handler != nullptr);

  LSErrorSafe err;
  ::LSMessageToken token = 0;
  const bool result =
      ::LSCall(handle_, kSignalUri, payload, filterMessage, handler, &token, &err);

  if (result)
    handler->SetToken(token);
  else {
    logError(err);
    token = 0;
  }
  return token;
}

void LunaClient::Cancel(unsigned int token) {
  if (token == 0)
    return;

  LSErrorSafe err;
  if (!::LSCallCancel(handle_, token, &err))
    logError(err);
}

bool LunaClient::Initialize(const char* id) {
  LSErrorSafe err;

  if (::LSRegister(id, &handle_, &err) &&
      ::LSGmainContextAttach(handle_, g_main_context_default(), &err) &&
      ::LSGmainSetPriority(handle_, G_PRIORITY_DEFAULT, &err))
    return true;

  logError(err);
  return false;
}

} //  namepspace luna
