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

#ifndef IPC_NEVA_REDEFINED_PARAM_TRAITS_MACROS_H_
#define IPC_NEVA_REDEFINED_PARAM_TRAITS_MACROS_H_

#include <type_traits>

// TODO(sergey.kipet@lge.com): file is to be deleted once the legacy
// Chromium IPC mechanism is finally dropped.

// Redefined convenience macro for defining enumerated type traits for types
// which are range-checked by the IPC system to be in the range of min_value..
// max_value inclusive.
#undef  IPC_ENUM_TRAITS_MIN_MAX_VALUE
#define IPC_ENUM_TRAITS_MIN_MAX_VALUE(enum_name, min_value, max_value)    \
  IPC_ENUM_TRAITS_VALIDATE(                                               \
      enum_name,                                                          \
      (static_cast<std::underlying_type<enum_name>::type>(value) >=       \
         static_cast<std::underlying_type<enum_name>::type>(min_value) && \
       static_cast<std::underlying_type<enum_name>::type>(value) <=       \
         static_cast<std::underlying_type<enum_name>::type>(max_value)))

#endif  // IPC_NEVA_REDEFINED_PARAM_TRAITS_MACROS_H_
