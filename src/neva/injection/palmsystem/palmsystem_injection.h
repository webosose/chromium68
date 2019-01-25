// Copyright 2014 LG Electronics, Inc.
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

#ifndef NEVA_INJECTION_PALMSYSTEM_PALMSYSTEM_INJECTION_H_
#define NEVA_INJECTION_PALMSYSTEM_PALMSYSTEM_INJECTION_H_

#include "base/compiler_specific.h"
#include "injection/common/public/renderer/injection_data_manager.h"
#include "v8/include/v8.h"

#if defined(COMPONENT_BUILD)

#if defined(PALMSYSTEM_IMPLEMENTATION)
#define PALMSYSTEM_EXPORT __attribute__((visibility("default")))
#else  // !defined(PALMSYSTEM_IMPLEMENTATION)
#define PALMSYSTEM_EXPORT
#endif  // defined(PALMSYSTEM_IMPLEMENTATION)

#else  // !defined(COMPONENT_BUILD)
#define PALMSYSTEM_EXPORT
#endif  // defined(COMPONENT_BUILD)

namespace extensions_v8 {

class InjectionWrapper;

class PALMSYSTEM_EXPORT PalmSystemInjectionExtension {
 public:
  static InjectionWrapper* Get();

  static const char kPalmSystemInjectionName[];
};

}  // namespace extensions_v8

#endif  // NEVA_INJECTION_PALMSYSTEM_PALMSYSTEM_INJECTION_H_
