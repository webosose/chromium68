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

#include "net/base/network_change_notifier_neva.h"

namespace neva {

NetworkChangeNotifierNeva::NetworkChangeNotifierNeva()
    : net::NetworkChangeNotifierLinux(std::unordered_set<std::string>()),
      network_connected_(true) {}

NetworkChangeNotifierNeva::~NetworkChangeNotifierNeva() {}

net::NetworkChangeNotifier::ConnectionType
NetworkChangeNotifierNeva::GetCurrentConnectionType() const {
  if (network_connected_)
    return net::NetworkChangeNotifierLinux::GetCurrentConnectionType();
  return net::NetworkChangeNotifier::CONNECTION_NONE;
}

void NetworkChangeNotifierNeva::OnNetworkStateChanged(bool is_connected) {
  bool was_connected = network_connected_;
  network_connected_ = is_connected;

  double max_bandwidth;
  ConnectionType connection_type;
  if (is_connected) {
    GetCurrentMaxBandwidthAndConnectionType(&max_bandwidth, &connection_type);
  } else {
    // In DNS failure webOS gives is_connected is false. But we get connected
    // as true from NetworkChangeNotifierLinux. So set it to CONNECTION_NONE
    max_bandwidth = 0.0;
    connection_type = net::NetworkChangeNotifier::CONNECTION_NONE;
  }

  if (was_connected != is_connected) {
    net::NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(
        max_bandwidth, connection_type);
  }
}

}  // namespace neva
