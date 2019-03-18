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

#ifndef NEVA_APP_RUNTIME_BROWSER_WEBOS_WEBOS_LUNA_SERVICE_H_
#define NEVA_APP_RUNTIME_BROWSER_WEBOS_WEBOS_LUNA_SERVICE_H_

#include <lunaservice.h>
#include <string>
#include <vector>

#include "base/memory/ptr_util.h"
#include "base/memory/singleton.h"
#include "base/values.h"
#include "content/public/browser/luna_service_delegate.h"

namespace neva {

// WebOSLunaService for webapp.
class WebOSLunaService : public content::LunaServiceDelegate {
 public:
  static WebOSLunaService* GetInstance();

  class Delegate {
   public:
    virtual ~Delegate() {}

    virtual void NotifySystemLocale(const std::string&) {}
  };

  bool LunaServiceCall(const std::string& uri,
                       const base::Value* parameters,
                       LSFilterFunc callback,
                       void* context);
  void LunaServiceCancel(LSMessageToken* token);

  void Initialize();
  void Finalize();
  void LaunchSettingsApplication(int target_id);
  void SetDelegate(Delegate* delegate) { delegate_ = delegate; }

  // content::LunaServiceDelegate
  LSHandle* GetHandle() override { return handle_; }

 private:
  friend struct base::DefaultSingletonTraits<WebOSLunaService>;

  // This object is is a singleton
  WebOSLunaService();
  virtual ~WebOSLunaService();

  Delegate* delegate_;

  LSHandle* handle_;
  GMainContext* context_;

  bool initialized_;

  // For unsubscribe lscall at finalize.
  // If there is some unsubscribe lscall at any time, Don't use this.
  // Single token should initialize with LSMESSAGE_TOKEN_INVALID
  std::vector<LSMessageToken> ls_token_vector_;

  void GetSystemSettings();

  void NotifySystemLocale(const std::string& ui);

  // LunaServiceCallBack
  static bool GetSystemSettingsCb(LSHandle* handle,
                                  LSMessage* reply,
                                  void* context);
  static bool LaunchApplicationStatusCb(LSHandle* handle,
                                        LSMessage* reply,
                                        void* context);

  DISALLOW_COPY_AND_ASSIGN(WebOSLunaService);
};

}  // namespace neva

#endif  // NEVA_APP_RUNTIME_BROWSER_WEBOS_WEBOS_LUNA_SERVICE_H_
