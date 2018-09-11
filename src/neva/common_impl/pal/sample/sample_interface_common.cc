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

#include "common_impl/pal/sample/sample_interface_common.h"

#include "base/threading/thread_task_runner_handle.h"
#include "content/public/browser/browser_thread.h"
#include "emulator/emulator_urls.h"

namespace pal {

SampleInterfaceCommon::SampleInterfaceCommon()
    : process_data_req_id_(0) {
  LOG(INFO) << __func__ << "() called";
  EmulatorDataSource* pEmulatorInterface = EmulatorDataSource::GetInstance();

  // adding all URLs (to different entities of the Sample injection verified)
  const char* poll_urls[] = {
    kSample_getPlatformValue,
    kSample_sampleUpdate,
    kSample_processDataResponse
  };
  for (auto* url: poll_urls) {
    pEmulatorInterface->AddURLForPolling(url, this,
        base::ThreadTaskRunnerHandle::Get());
  }
}

SampleInterfaceCommon::~SampleInterfaceCommon() {}

std::unique_ptr<SampleInterface::SampleUpdateSubscription>
SampleInterfaceCommon::AddCallback(
    const SampleInterface::SampleUpdateCallback& callback) {
  LOG(INFO) << __func__ << "(): called";

  return callbacks_.Add(callback);
}

void SampleInterfaceCommon::CallFunc(std::string arg1, std::string arg2) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): arg1 = " << arg1 << ", arg2 = " << arg2;

  RequestArgumentDescription args[] = {{"arg1", &arg1}, {"arg2", &arg2}};
  RequestArgs args_vector(&args[0], &args[2]);
  std::string params = EmulatorDataSource::PrepareRequestParams(args_vector);
  EmulatorDataSource::SetExpectationAsync(kSample_callFunc, params);
}

std::string SampleInterfaceCommon::GetCurrentValue() const {
  LOG(INFO) << __func__ << "(): called";
  return 0;
}

std::string SampleInterfaceCommon::GetPlatformValue() const {
  LOG(INFO) << __func__ << "(): called";
  return EmulatorDataSource::GetInstance()->GetCachedValueForURL(
      kSample_getPlatformValue);
}

void SampleInterfaceCommon::ProcessData(
    const std::string data,
    const ProcessDataRespondCallback& on_process_data_done) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): data = " << data;

  std::string id(std::to_string(++process_data_req_id_));
  process_data_requests_.insert(ProcessDataRequest(process_data_req_id_,
      std::move(on_process_data_done)));
  RequestArgumentDescription args[] = {{"id", &id}, {"data", &data}};
  RequestArgs args_vector(&args[0], &args[2]);
  std::string params = EmulatorDataSource::PrepareRequestParams(args_vector);
  EmulatorDataSource::SetExpectationAsync(kSample_processDataReq, params);
}

void SampleInterfaceCommon::onProcessDataResponse(const std::string& data) {
  std::string id;
  std::string result;
  ResponseArgumentDescription args[] = {{"id", &id}, {"result", &result}};
  ResponseArgs args_vector(&args[0], &args[2]);

  if (!EmulatorDataSource::GetResponseParams(args_vector, data))
    return;

  ProcessDataRequestsIterator it = process_data_requests_.find(
      std::stoi(id));
  if (it == process_data_requests_.end()) {
    LOG(ERROR) << __func__ << "(): incorrect response id = " << id;
    return;
  }
  ProcessDataRespondCallback process_data_done_cb = std::move(it->second);
  process_data_requests_.erase(it);
  bool res = result != "false";
  if (!process_data_done_cb.is_null()) {
    process_data_done_cb.Run(res);
    process_data_done_cb.Reset();
  }
}

void SampleInterfaceCommon::DataUpdated(const std::string& url,
                                    const std::string& data) {
  LOG(INFO) << __func__ << "(): called";
  LOG(INFO) << __func__ << "(): url = " << url << ", data = " << data;
  if (url.compare(kSample_sampleUpdate) == 0) {
    callbacks_.Notify(data);
  } else if (url.compare(kSample_processDataResponse) == 0) {
    onProcessDataResponse(data);
  }
}
}  // namespace pal
