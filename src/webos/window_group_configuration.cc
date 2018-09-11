// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "webos/window_group_configuration.h"

namespace webos {

WindowGroupConfiguration::WindowGroupConfiguration(const std::string& name)
    : name_(name), is_anonymous_(false) {}

WindowGroupConfiguration::~WindowGroupConfiguration() {}

void WindowGroupConfiguration::SetIsAnonymous(bool is_anonymous) {
  is_anonymous_ = is_anonymous;
}

bool WindowGroupConfiguration::GetIsAnonymous() const {
  return is_anonymous_;
}

std::string WindowGroupConfiguration::GetName() const {
  return name_;
}

void WindowGroupConfiguration::AddLayer(
    const WindowGroupLayerConfiguration& layer_config) {
  layers_.push_back(layer_config);
}

const std::vector<WindowGroupLayerConfiguration>&
WindowGroupConfiguration::GetLayers() const {
  return layers_;
}

}  // namespace webos
