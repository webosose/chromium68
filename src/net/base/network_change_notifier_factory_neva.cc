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

#include "net/base/network_change_notifier_factory_neva.h"

#include "net/base/network_change_notifier_neva.h"

namespace neva {

namespace {

NetworkChangeNotifierNeva* g_network_change_notifier_neva = nullptr;

}  // namespace

net::NetworkChangeNotifier* NetworkChangeNotifierFactoryNeva::CreateInstance() {
  if (!g_network_change_notifier_neva)
    g_network_change_notifier_neva = new NetworkChangeNotifierNeva();
  return g_network_change_notifier_neva;
}

// static
NetworkChangeNotifierNeva* NetworkChangeNotifierFactoryNeva::GetInstance() {
  return g_network_change_notifier_neva;
}

}  // namespace neva
