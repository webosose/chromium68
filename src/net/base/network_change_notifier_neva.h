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

#ifndef NET_BASE_NETWORK_CHANGE_NOTIFIER_NEVA_H_
#define NET_BASE_NETWORK_CHANGE_NOTIFIER_NEVA_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/base/network_change_notifier_linux.h"

namespace neva {

class NET_EXPORT_PRIVATE NetworkChangeNotifierNeva
    : public net::NetworkChangeNotifierLinux {
 public:
  NetworkChangeNotifierNeva();
  ~NetworkChangeNotifierNeva() override;

  void OnNetworkStateChanged(bool is_connected);

  // net::NetworkChangeNotifierLinux overrides.
  net::NetworkChangeNotifier::ConnectionType GetCurrentConnectionType()
      const override;

 private:
  bool network_connected_;

  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierNeva);
};

}  // namespace neva

#endif  // NET_BASE_NETWORK_CHANGE_NOTIFIER_NEVA_H_
