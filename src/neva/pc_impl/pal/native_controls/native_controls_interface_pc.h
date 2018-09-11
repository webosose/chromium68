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

#ifndef PC_IMPL_PAL_NATIVE_CONTROLS_INTERFACE_PC_H_
#define PC_IMPL_PAL_NATIVE_CONTROLS_INTERFACE_PC_H_

#include "emulator/emulator_data_source.h"
#include "pal/public/interfaces/native_controls_interface.h"

namespace pal {

using namespace emulator;

class NativeControlsInterfacePC : public NativeControlsInterface,
                                  public EmulatorDataDelegate {
 public:
  NativeControlsInterfacePC();
  ~NativeControlsInterfacePC() override;

  void OpenColorChooser(
      const std::string& color_params,
      const OpenColorChooserRespondCallback& on_done) override;

  void CloseColorChooser() override;

  void RunFileChooser(int mode, const std::string& title,
                      const std::string& default_file_name,
                      const std::vector<std::string>& accept_types,
                      bool need_local_path, const std::string& url,
                      const RunFileChooserRespondCallback& callback) override;

  std::unique_ptr<ColorChosenSubscription> AddCallback(
      const ColorChosenCallback& callback) override;

 private:
  void ChangedValueColorChooser(const std::string& data);

  // from EmulatorDataDelegate
  void DataUpdated(const std::string& url, const std::string& data) override;

  ColorChosenCallbackList color_choosen_callbacks_;
  OpenColorChooserRespondCallback color_respond_callback_;
  RunFileChooserRespondCallback file_respond_callback_;
};

}  // namespace pal

#endif  // PC_IMPL_PAL_NATIVE_CONTROLS_INTERFACE_PC_H_
