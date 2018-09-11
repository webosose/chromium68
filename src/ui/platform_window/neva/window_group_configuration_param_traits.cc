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

#include "ui/platform_window/neva/window_group_configuration_param_traits.h"

// Generate param traits write methods.
#include "ipc/param_traits_write_macros.h"
namespace IPC {
#undef UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_MACROS_H_
#include "ui/platform_window/neva/window_group_configuration_param_traits_macros.h"
}  // namespace IPC

// Generate param traits read methods.
#include "ipc/param_traits_read_macros.h"
namespace IPC {
#undef UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_MACROS_H_
#include "ui/platform_window/neva/window_group_configuration_param_traits_macros.h"
}  // namespace IPC

// Generate param traits log methods.
#include "ipc/param_traits_log_macros.h"
namespace IPC {
#undef UI_PLATFORM_WINDOW_WEBOS_IPC_WINDOW_GROUP_CONFIGURATION_PARAM_TRAITS_MACROS_H_
#include "ui/platform_window/neva/window_group_configuration_param_traits_macros.h"
}  // namespace IPC

namespace IPC {

void ParamTraits<ui::WindowGroupConfiguration>::Write(base::Pickle* m,
                                                      const param_type& p) {
  WriteParam(m, p.name);
  WriteParam(m, p.is_anonymous);

  uint32_t size = static_cast<uint32_t>(p.layers.size());
  WriteParam(m, size);
  for (size_t i = 0; i < size; i++) {
    WriteParam(m, p.layers[i].name);
    WriteParam(m, p.layers[i].z_order);
  }
}

bool ParamTraits<ui::WindowGroupConfiguration>::Read(const base::Pickle* m,
                                                     base::PickleIterator* iter,
                                                     param_type* p) {
  if (!ReadParam(m, iter, &p->name))
    return false;
  if (!ReadParam(m, iter, &p->is_anonymous))
    return false;

  uint32_t size;
  if (!ReadParam(m, iter, &size))
    return false;
  for (size_t i = 0; i < size; i++) {
    ui::WindowGroupLayerConfiguration layer_config;
    if (!ReadParam(m, iter, &layer_config.name))
      return false;
    if (!ReadParam(m, iter, &layer_config.z_order))
      return false;
    p->layers.push_back(layer_config);
  }
  return true;
}

void ParamTraits<ui::WindowGroupConfiguration>::Log(const param_type& p,
                                                    std::string* l) {
  LogParam(p.name, l);
  l->append(" ");
  LogParam(p.is_anonymous, l);
  l->append(" ");

  uint32_t size = static_cast<uint32_t>(p.layers.size());
  LogParam(size, l);
  l->append(" ");
  for (size_t i = 0; i < size; i++) {
    LogParam(p.layers[i].name, l);
    l->append(" ");
    LogParam(p.layers[i].z_order, l);
    l->append(" ");
  }
}

}  // namespace IPC
