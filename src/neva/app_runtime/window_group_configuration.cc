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

#include "neva/app_runtime/public/window_group_configuration.h"
#include "ui/platform_window/neva/window_group_configuration.h"

namespace app_runtime {

WindowGroupLayerConfiguration::WindowGroupLayerConfiguration() : z_order(0) {}

WindowGroupLayerConfiguration::WindowGroupLayerConfiguration(
    const WindowGroupLayerConfiguration& other) = default;

WindowGroupConfiguration::WindowGroupConfiguration() : is_anonymous(false) {}
WindowGroupConfiguration::WindowGroupConfiguration(
    const WindowGroupConfiguration&) = default;
WindowGroupConfiguration::~WindowGroupConfiguration() = default;

WindowGroupLayerConfiguration::operator ui::WindowGroupLayerConfiguration()
    const {
  ui::WindowGroupLayerConfiguration* f =
      new ui::WindowGroupLayerConfiguration();

  f->name = name;
  f->z_order = z_order;
  return *f;
}

WindowGroupConfiguration::operator ui::WindowGroupConfiguration() const {
  ui::WindowGroupConfiguration* f = new ui::WindowGroupConfiguration();

  f->name = name;
  f->is_anonymous = is_anonymous;

  for (auto& it : layers) {
    ui::WindowGroupLayerConfiguration layer_config = it;
    f->layers.push_back(layer_config);
  }

  return *f;
}

}  // namespace app_runtime
