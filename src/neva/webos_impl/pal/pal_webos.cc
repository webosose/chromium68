// Copyright 2018 LG Electronics, Inc.
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

#include <mutex>

#include "common_impl/pal/sample/sample_interface_common.h"
#include "webos_impl/pal/memorymanager/memorymanager_interface_webos.h"
#include "webos_impl/pal/pal_webos.h"
#include "webos_impl/pal/systemlocale/systemlocale_interface_webos.h"

namespace pal {

PalWebOS::PalWebOS() {}

PalWebOS::~PalWebOS() {}

MemoryManagerInterface* PalWebOS::GetMemoryManagerInterface() {
  if (memoryManagerInterfaceInstance_ == nullptr)
    memoryManagerInterfaceInstance_.reset(new MemoryManagerInterfaceWebOS());
  return memoryManagerInterfaceInstance_.get();
}

SampleInterface* PalWebOS::GetSampleInterface() {
  if (sampleInterfaceInstance_ == nullptr)
    sampleInterfaceInstance_.reset(new SampleInterfaceCommon());
  return sampleInterfaceInstance_.get();
}

SystemLocaleInterface* PalWebOS::GetSystemLocaleInterface() {
  if (systemLocaleInterfaceInstance_ == nullptr)
    systemLocaleInterfaceInstance_.reset(new SystemLocaleInterfaceWebOS());
  return systemLocaleInterfaceInstance_.get();
}

std::unique_ptr<Pal> PalInstance;
std::once_flag pal_pc_instance_flag;

Pal* Pal::GetPlatformInstance() {
  std::call_once(pal_pc_instance_flag,
                 []() { PalInstance.reset(new PalWebOS()); });
  return PalInstance.get();
}

}  // namespace pal
