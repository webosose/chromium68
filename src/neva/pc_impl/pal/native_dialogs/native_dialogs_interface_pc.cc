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

#include "pc_impl/pal/native_dialogs/native_dialogs_interface_pc.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "emulator/emulator_urls.h"

namespace pal {

NativeDialogsInterfacePC::NativeDialogsInterfacePC() {
  LOG(INFO) << __func__ << "() called";
  EmulatorDataSource* pEmulatorInterface = EmulatorDataSource::GetInstance();

  // adding all URLs (to different entities of the Sample injection verified)
  const char* poll_urls[] = {kNativeDialogs_retJavaScriptDialog};
  for (auto* url : poll_urls) {
    pEmulatorInterface->AddURLForPolling(url, this,
                                         base::ThreadTaskRunnerHandle::Get());
  }
}

NativeDialogsInterfacePC::~NativeDialogsInterfacePC() {}

void NativeDialogsInterfacePC::CancelJavaScriptDialog() {
  LOG(INFO) << __func__ << "(): called";
  EmulatorDataSource::SetExpectationAsync(
      kNativeDialogs_cancelJavaScriptDialog, "");
  return;
}

void NativeDialogsInterfacePC::RunJavaScriptDialog(
    int dialog_type, const std::string& message_text,
    const std::string& default_prompt_text, const std::string& url,
    const RunJavaScriptDialogRespondCallback& callback) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): dialog_type = " << dialog_type
            << ", message_text = " << message_text
            << ", default_prompt_text = " << default_prompt_text
            << ", url = " << url;

  std::string dlg_type_str = std::to_string(dialog_type);
  RequestArgumentDescription args[] = {
      {"dialog_type", &dlg_type_str},
      {"message_text", &message_text},
      {"default_prompt_text", &default_prompt_text},
      {"url", &url}};
  RequestArgs args_vector(&args[0], &args[4]);
  std::string params = EmulatorDataSource::PrepareRequestParams(args_vector);
  cb_ = std::move(callback);
  EmulatorDataSource::SetExpectationAsync(kNativeDialogs_runJavaScriptDialog,
                                          params);
  return;
}

void NativeDialogsInterfacePC::DataUpdated(const std::string& url,
                                           const std::string& data) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): url = " << url << ", data = " << data;
  if (url.compare(kNativeDialogs_retJavaScriptDialog) == 0) {
    std::string success;
    std::string dialog_type;
    std::string input_text;

    ResponseArgumentDescription args[] = {{"success", &success},
                                          {"dialog_type", &dialog_type},
                                          {"input_text", &input_text}};
    ResponseArgs args_vector(&args[0], &args[3]);

    if (!EmulatorDataSource::GetResponseParams(args_vector, data)) return;
    bool res_success = success != "false";
    if (!cb_.is_null()) cb_.Run(res_success, input_text);
  }
}

}  // namespace pal
