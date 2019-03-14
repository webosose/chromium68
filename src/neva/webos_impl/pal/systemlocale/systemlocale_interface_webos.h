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

#ifndef NEVA_WEBOS_IMPL_PAL_SYSTEMLOCALE_SYSTEMLOCALE_INTERFACE_WEBOS_H_
#define NEVA_WEBOS_IMPL_PAL_SYSTEMLOCALE_SYSTEMLOCALE_INTERFACE_WEBOS_H_

#include "pal/public/interfaces/system_locale_interface.h"

#include <list>
#include <memory>
#include <queue>

namespace lunabus {
class Handler;
class LunaClient;
}

namespace pal {

class SystemLocaleInterfaceWebOS : public SystemLocaleInterface {
 public:
  SystemLocaleInterfaceWebOS();
  ~SystemLocaleInterfaceWebOS() override;

  // Acquires app id seeking for system locale info (to be called very first).
  bool Initialize(const char* identifier) override;
  // Cancels subscription to system locale info update (to be called very last).
  void Shutdown() override;

  // Registers callback that needs to be run on system locale info update.
  std::unique_ptr<SystemLocaleChangedSubscription> AddCallback(
      const SystemLocaleChangedCallback& callback) override;

  // Notifies registered callbacks on system locale info update.
  void OnLocaleInfoChanged(const char* locale_info_json);

 private:
  std::unique_ptr<lunabus::LunaClient> lunaclient_;
  std::unique_ptr<lunabus::Handler> localeInfoChangedHandler_;

  SystemLocaleChangedCallbackList localeInfoChangedCallbacks_;
};

}  // namespace pal

#endif  // NEVA_WEBOS_IMPL_PAL_SYSTEMLOCALE_SYSTEMLOCALE_INTERFACE_WEBOS_H_
