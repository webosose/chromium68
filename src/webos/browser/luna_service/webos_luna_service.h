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

#ifndef WEBOS_BROWSER_LUNA_SERVICE_WEBOS_LUNA_SERVICE_H_
#define WEBOS_BROWSER_LUNA_SERVICE_WEBOS_LUNA_SERVICE_H_

#include <lunaservice.h>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "webos/public/runtime_delegates.h"

namespace webos {

// WebOSLunaService for webapp.
class WebOSLunaService : public LunaServiceDelegate {
 public:
  static WebOSLunaService* GetInstance();

  bool LunaServiceCall(const std::string& uri,
                       const base::Value* parameters,
                       LSFilterFunc callback,
                       void* context);
  void LunaServiceCancel(LSMessageToken* token);

  void Initialize();
  void Finalize();
  void LaunchSettingsApplication(int target_id);

  LSHandle* GetLSHandle() override { return handle_; }

 private:
  friend struct base::DefaultSingletonTraits<WebOSLunaService>;

  // This object is is a singleton
  WebOSLunaService();
  virtual ~WebOSLunaService();

  LSHandle* handle_;
  GMainContext* context_;

  bool initialized_;

  // For unsubscribe lscall at finalize.
  // If there is some unsubscribe lscall at any time, Don't use this.
  // Single token should initialize with LSMESSAGE_TOKEN_INVALID
  std::vector<LSMessageToken> ls_token_vector_;

  void GetSystemSettings();

  // LunaServiceCallBack
  static bool GetSystemSettingsCb(LSHandle* handle,
                                  LSMessage* reply,
                                  void* context);
  static bool LaunchApplicationStatusCb(LSHandle* handle,
                                        LSMessage* reply,
                                        void* context);

  DISALLOW_COPY_AND_ASSIGN(WebOSLunaService);
};

}  // namespace webos

#endif  // WEBOS_BROWSER_LUNA_SERVICE_WEBOS_LUNA_SERVICE_H_
