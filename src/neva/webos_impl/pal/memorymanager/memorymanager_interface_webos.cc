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

#include "webos_impl/pal/memorymanager/memorymanager_interface_webos.h"

#include <string>

#include "base/json/json_reader.h"
#include "base/json/string_escape.h"
#include "neva/webos_impl/luna/luna_client.h"

namespace {

const char kIdentifier[] = "com.webos.browser.memorymanager.client";
const char kMemoryManagerId[] = "com.webos.memorymanager";
const char kSubscribeToThresholdChanged[] =
    R"JSON({"category":"/com/webos/memory", "method":"thresholdChanged"})JSON";
const char kGetMemoryStatusMethod[] =
    "luna://com.webos.memorymanager/getCurrentMemState";
const char kGetMemoryStatusRequest[] = "{}";

class LevelChangedHandler : public lunabus::Handler {
 public:
  LevelChangedHandler(pal::MemoryManagerInterfaceWebOS& memorymanager)
    : memorymanager_(memorymanager) {
  }

  void Handle(const char* payload) override {
    memorymanager_.OnLevelChanged(payload);
  }

 private:
  pal::MemoryManagerInterfaceWebOS& memorymanager_;
};

class MemoryStatusHandler : public lunabus::Handler {
 public:
  MemoryStatusHandler(pal::MemoryManagerInterfaceWebOS& memorymanager)
    : memorymanager_(memorymanager) {
  }

  void Handle(const char* payload) override {
    memorymanager_.OnMemoryStatusRespond(payload);
  }

 private:
  pal::MemoryManagerInterfaceWebOS& memorymanager_;
};

}  // namepsace

namespace pal {

MemoryManagerInterfaceWebOS::MemoryManagerInterfaceWebOS() {
  Initialize();
}

MemoryManagerInterfaceWebOS::~MemoryManagerInterfaceWebOS() {
  if (!lunaclient_)
    return;
  if (levelChangedHandler_)
    lunaclient_->Cancel(levelChangedHandler_->GetToken());
  if (memoryStatusHandler_)
    lunaclient_->Cancel(memoryStatusHandler_->GetToken());
}

void MemoryManagerInterfaceWebOS::Initialize() {
  lunaclient_ = std::make_unique<lunabus::LunaClient>(kIdentifier);
  if (lunaclient_->Initialized()) {
    levelChangedHandler_ = std::make_unique<LevelChangedHandler>(*this);
    memoryStatusHandler_ = std::make_unique<MemoryStatusHandler>(*this);
    lunaclient_->Signal(
        kSubscribeToThresholdChanged, levelChangedHandler_.get());
  }
}

void MemoryManagerInterfaceWebOS::OnLevelChanged(const char* level_json) {
  levelChangedCallbacks_.Notify(std::string(level_json));
}

void MemoryManagerInterfaceWebOS::OnMemoryStatusRespond(const char* status_json) {
  if (memoryStatusCallbacks_.empty())
    return;

  GetMemoryStatusRespondCallback& callback = memoryStatusCallbacks_.front();
  callback.Run(std::string(status_json));
  memoryStatusCallbacks_.pop();
}

std::unique_ptr<MemoryManagerInterface::LevelChangedSubscription>
    MemoryManagerInterfaceWebOS::AddCallback(
        const LevelChangedCallback& callback) {
  return levelChangedCallbacks_.Add(callback);
}

void MemoryManagerInterfaceWebOS::GetMemoryStatus(
    const GetMemoryStatusRespondCallback& on_done) {
  if (lunaclient_ && lunaclient_->Initialized()) {
    memoryStatusCallbacks_.push(on_done);
    lunaclient_->Call(kGetMemoryStatusMethod,
                      kGetMemoryStatusRequest,
                      memoryStatusHandler_.get());
  }
}

}  // namespace pal
