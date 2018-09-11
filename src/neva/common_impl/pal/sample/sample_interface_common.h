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

#ifndef PC_IMPL_PAL_SAMPLE_SAMPLE_INTERFACE_COMMON_H_
#define PC_IMPL_PAL_SAMPLE_SAMPLE_INTERFACE_COMMON_H_

#include "pal/public/interfaces/sample_interface.h"
#include "emulator/emulator_data_source.h"

namespace pal {

using namespace emulator;

class SampleInterfaceCommon : public SampleInterface,
                          public EmulatorDataDelegate {
 public:
  SampleInterfaceCommon();
  ~SampleInterfaceCommon() override;

  using ProcessDataRequest = std::pair<int, ProcessDataRespondCallback>;
  using ProcessDataRequests = std::map<int, ProcessDataRespondCallback>;
  using ProcessDataRequestsIterator = ProcessDataRequests::iterator;

  std::unique_ptr<SampleUpdateSubscription> AddCallback(
      const SampleUpdateCallback& callback) override;
  void CallFunc(const std::string arg1, const std::string arg2) override;
  std::string GetCurrentValue() const override;
  std::string GetPlatformValue() const override;
  void ProcessData(
      const std::string data,
      const ProcessDataRespondCallback& on_process_data_done) override;

 private:
  // from EmulatorDataDelegate
  void DataUpdated(const std::string& url, const std::string& data) override;
  void onProcessDataResponse(const std::string& data);

  SampleUpdateCallbackList callbacks_;
  int process_data_req_id_;
  ProcessDataRequests process_data_requests_;
};

}  // namespace pal

#endif  // PC_IMPL_PAL_SAMPLE_SAMPLE_INTERFACE_COMMON_H_
