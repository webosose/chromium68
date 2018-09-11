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

#ifndef PC_IMPL_PAL_NATIVE_DIALOGS_INTERFACE_PC_H_
#define PC_IMPL_PAL_NATIVE_DIALOGS_INTERFACE_PC_H_

#include "pal/public/interfaces/native_dialogs_interface.h"
#include "emulator/emulator_data_source.h"

namespace pal {

using namespace emulator;

class NativeDialogsInterfacePC : public NativeDialogsInterface,
                          public EmulatorDataDelegate {
 public:
  NativeDialogsInterfacePC();
  ~NativeDialogsInterfacePC() override;

  using RunJavaScriptDialogRespondCallback =
      base::Callback<void(bool success, std::string& user_input)>;

  void CancelJavaScriptDialog() override;
  void RunJavaScriptDialog(int dialog_type,
                           const std::string& message_text,
                           const std::string& default_prompt_text,
                           const std::string& url,
                           const RunJavaScriptDialogRespondCallback&
                           callback) override;

 private:
  // from EmulatorDataDelegate
  void DataUpdated(const std::string& url, const std::string& data) override;
  RunJavaScriptDialogRespondCallback cb_;
};

}  // namespace pal

#endif  // PC_IMPL_PAL_NATIVE_DIALOGS_INTERFACE_PC_H_
