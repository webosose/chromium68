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

#include "injection/common/public/renderer/injection_loader_extension.h"

#include "base/logging.h"

#if defined(OS_WEBOS)
#include "neva/injection/webossystem/webossystem_injection.h"
#endif

#if defined(ENABLE_SAMPLE_WEBAPI)
#include "injection/sample/sample_injection.h"
#endif

#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "injection/browser_control/browser_control_injection.h"
#endif

#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "injection/memorymanager/memorymanager_injection.h"
#endif

namespace extensions_v8 {

InjectionWrapper* InjectionLoaderExtension::Get(const std::string& name) {
#if defined(ENABLE_SAMPLE_WEBAPI)
  if (name == extensions_v8::SampleInjectionExtension::kInjectionName)
    return extensions_v8::SampleInjectionExtension::Get();
#endif
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  if (name == extensions_v8::BrowserControlInjectionExtension::kInjectionName)
    return extensions_v8::BrowserControlInjectionExtension::Get();
#endif
#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
  if (name ==
      extensions_v8::MemoryManagerInjectionExtension::kInjectionName)
    return extensions_v8::MemoryManagerInjectionExtension::Get();
#endif
  return nullptr;
}

InjectionInstallFunction InjectionLoaderExtension::GetInjectionInstallFunction(
    const std::string& name) {
#if defined(OS_WEBOS)
  if (name ==
      extensions_v8::WebOSSystemInjectionExtension::kWebOSSystemInjectionName)
    return extensions_v8::WebOSSystemInjectionExtension::Install;
  NOTREACHED() << "Non-existing injection " << name;
#endif
  return nullptr;
}

}  // namespace extensions_v8
