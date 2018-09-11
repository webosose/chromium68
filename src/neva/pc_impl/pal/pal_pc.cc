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

#include <mutex>

#include "pc_impl/pal/pal_pc.h"

#include "pc_impl/pal/native_controls/native_controls_interface_pc.h"
#include "pc_impl/pal/native_dialogs/native_dialogs_interface_pc.h"
#include "common_impl/pal/sample/sample_interface_common.h"

namespace pal {

PalPC::PalPC() {}

PalPC::~PalPC() {}

NativeControlsInterface* PalPC::GetNativeControlsInterface() {
  if (nativeConrolsInterfaceInstance_ == nullptr)
    nativeConrolsInterfaceInstance_.reset(new NativeControlsInterfacePC());
  return nativeConrolsInterfaceInstance_.get();
}

NativeDialogsInterface* PalPC::GetNativeDialogsInterface() {
  if (nativeDialogsInterfaceInstance_ == nullptr)
    nativeDialogsInterfaceInstance_.reset(new NativeDialogsInterfacePC());
  return nativeDialogsInterfaceInstance_.get();
}

SampleInterface* PalPC::GetSampleInterface() {
  if (sampleInterfaceInstance_ == nullptr)
    sampleInterfaceInstance_.reset(new SampleInterfaceCommon());
  return sampleInterfaceInstance_.get();
}

std::unique_ptr<Pal> PalInstance;
std::once_flag pal_pc_instance_flag;

Pal* Pal::GetPlatformInstance() {
  std::call_once(pal_pc_instance_flag,
                 []() { PalInstance.reset(new PalPC()); });
  return PalInstance.get();
}

}  // namespace pal
