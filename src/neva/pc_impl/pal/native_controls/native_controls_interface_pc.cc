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

#include "pc_impl/pal/native_controls/native_controls_interface_pc.h"

#include "base/threading/thread_task_runner_handle.h"
#include "base/values.h"
#include "emulator/emulator_urls.h"

namespace pal {

NativeControlsInterfacePC::NativeControlsInterfacePC() {
  LOG(INFO) << __func__ << "() called";
  EmulatorDataSource* pEmulatorInterface = EmulatorDataSource::GetInstance();

  // adding all URLs (to different entities of the Sample injection verified)
  const char* poll_urls[] = {
    kNativeControls_changedColorChooser,
    kNativeControls_closedColorChooser,
    kNativeControls_retFileChooser
  };

  for (auto* url : poll_urls) {
    pEmulatorInterface->AddURLForPolling(url, this,
                                         base::ThreadTaskRunnerHandle::Get());
  }
}

NativeControlsInterfacePC::~NativeControlsInterfacePC() {}

void NativeControlsInterfacePC::OpenColorChooser(
      const std::string& color_params,
      const OpenColorChooserRespondCallback& on_done) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): color_params = " << color_params;

  color_respond_callback_ = std::move(on_done);
  EmulatorDataSource::SetExpectationAsync(kNativeControls_openColorChooser,
                                          color_params);
  return;
}

void NativeControlsInterfacePC::CloseColorChooser() {
  LOG(INFO) << __func__ << "(): called";
  EmulatorDataSource::SetExpectationAsync(kNativeControls_closedColorChooser,
                                          "");
}

void NativeControlsInterfacePC::ChangedValueColorChooser(
    const std::string& data) {
  std::string color;
  ResponseArgumentDescription args[] = {{"color", &color}};
  ResponseArgs args_vector(&args[0], &args[1]);
  if (!EmulatorDataSource::GetResponseParams(args_vector, data))
    return;
  color_choosen_callbacks_.Notify(stoi(color));
}

void NativeControlsInterfacePC::RunFileChooser(
    int mode, const std::string& title, const std::string& default_file_name,
    const std::vector<std::string>& accept_types, bool need_local_path,
    const std::string& url, const RunFileChooserRespondCallback& callback) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): mode = " << mode << ", title = " << title
            << ", need_local_path = " << need_local_path << ", url = " << url;

  using namespace base;
  DictionaryValue request;
  request.SetInteger("mode", mode);
  request.SetString("title", title);
  request.SetString("default_file_name", default_file_name);
  request.SetBoolean("need_local_path", need_local_path);
  request.SetString("url", url);
  std::unique_ptr<ListValue> accept_types_list(new ListValue());
  for (auto accept_type : accept_types) {
    accept_types_list->AppendString(accept_type);
  }
  request.SetList("accept_types", std::move(accept_types_list));
  std::string params = EmulatorDataSource::PrepareRequestParams(request);

  file_respond_callback_ = std::move(callback);
  EmulatorDataSource::SetExpectationAsync(kNativeControls_runFileChooser,
                                          params);
  return;
}

void NativeControlsInterfacePC::DataUpdated(const std::string& url,
                                            const std::string& data) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): url = " << url << ", data = " << data;

  if (url.compare(kNativeControls_retFileChooser) == 0) {
    if (!file_respond_callback_.is_null()) file_respond_callback_.Run(data);
  } else if (url.compare(kNativeControls_changedColorChooser) == 0) {
    ChangedValueColorChooser(data);
  } else if (url.compare(kNativeControls_closedColorChooser) == 0) {
    if (!color_respond_callback_.is_null()) color_respond_callback_.Run();
  }
}

std::unique_ptr<NativeControlsInterface::ColorChosenSubscription>
NativeControlsInterfacePC::AddCallback(const ColorChosenCallback& callback) {
  return color_choosen_callbacks_.Add(callback);
}

}  // namespace pal
