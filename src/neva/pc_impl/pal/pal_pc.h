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

#ifndef NEVA_PC_IMPL_PAL_PAL_PC_H_
#define NEVA_PC_IMPL_PAL_PAL_PC_H_

#include <memory>
#include "pal/public/pal.h"

namespace pal {

class PalPC : public Pal {
 public:
  PalPC();
  ~PalPC() override;

  NativeControlsInterface* GetNativeControlsInterface() override;
  NativeDialogsInterface* GetNativeDialogsInterface() override;
  SampleInterface* GetSampleInterface() override;

 private:
  std::unique_ptr<NativeControlsInterface> nativeConrolsInterfaceInstance_;
  std::unique_ptr<NativeDialogsInterface> nativeDialogsInterfaceInstance_;
  std::unique_ptr<SampleInterface> sampleInterfaceInstance_;
};

}  // namespace pal

#endif  // NEVA_PC_IMPL_PAL_PAL_PC_H_
