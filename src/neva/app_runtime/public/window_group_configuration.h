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

#ifndef NEVA_APP_RUNTIME_PUBLIC_WINDOW_GROUP_CONFIGURATION_H
#define NEVA_APP_RUNTIME_PUBLIC_WINDOW_GROUP_CONFIGURATION_H

#include <string>
#include <vector>

#include "neva/app_runtime/public/app_runtime_export.h"

namespace ui {
class WindowGroupConfiguration;
class WindowGroupLayerConfiguration;
}

namespace app_runtime {

class APP_RUNTIME_EXPORT WindowGroupLayerConfiguration {
 public:
  WindowGroupLayerConfiguration();
  WindowGroupLayerConfiguration(const WindowGroupLayerConfiguration& other);

  std::string name;
  int z_order = 0;

  operator ui::WindowGroupLayerConfiguration() const;
};

class WindowGroupConfiguration {
 public:
  WindowGroupConfiguration();
  WindowGroupConfiguration(const WindowGroupConfiguration&);
  ~WindowGroupConfiguration();

  std::string name;
  bool is_anonymous = false;

  std::vector<WindowGroupLayerConfiguration> layers;

  operator ui::WindowGroupConfiguration() const;
};
}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_WINDOW_GROUP_CONFIGURATION_H
