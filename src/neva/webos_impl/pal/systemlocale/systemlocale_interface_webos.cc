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

#include "webos_impl/pal/systemlocale/systemlocale_interface_webos.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/string_escape.h"
#include "neva/webos_impl/luna/luna_client.h"

namespace {

const char kGetLocaleInfoMethod[] =
    "palm://com.webos.settingsservice/getSystemSettings";
const char kGetLocaleInfoRequest[] =
    R"JSON({"keys":["localeInfo"], "subscribe":true})JSON";

class LocaleInfoChangedHandler : public lunabus::Handler {
 public:
  LocaleInfoChangedHandler(pal::SystemLocaleInterfaceWebOS& systemlocale)
      : systemlocale_(systemlocale) {}

  void Handle(const char* payload) override {
    systemlocale_.OnLocaleInfoChanged(payload);
  }

 private:
  pal::SystemLocaleInterfaceWebOS& systemlocale_;
};

}  // namespace

namespace pal {

SystemLocaleInterfaceWebOS::SystemLocaleInterfaceWebOS() {}
SystemLocaleInterfaceWebOS::~SystemLocaleInterfaceWebOS() {}

bool SystemLocaleInterfaceWebOS::Initialize(const char* identifier) {
  bool result = false;

  lunaclient_ = std::make_unique<lunabus::LunaClient>(identifier);

  if (lunaclient_->Initialized()) {
    localeInfoChangedHandler_ =
        std::make_unique<LocaleInfoChangedHandler>(*this);

    lunaclient_->Call(kGetLocaleInfoMethod, kGetLocaleInfoRequest,
                      localeInfoChangedHandler_.get(), true);

    result = true;
  }

  return result;
}

void SystemLocaleInterfaceWebOS::Shutdown() {
  if (!lunaclient_)
    return;

  if (localeInfoChangedHandler_)
    lunaclient_->Cancel(localeInfoChangedHandler_->GetToken());
}

std::unique_ptr<SystemLocaleInterface::SystemLocaleChangedSubscription>
SystemLocaleInterfaceWebOS::AddCallback(
    const SystemLocaleChangedCallback& callback) {
  return localeInfoChangedCallbacks_.Add(callback);
}

void SystemLocaleInterfaceWebOS::OnLocaleInfoChanged(
    const char* locale_info_json) {
  localeInfoChangedCallbacks_.Notify(locale_info_json);
}

}  // namespace pal
