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

#ifndef NEVA_WEBOS_IMPL_PAL_MEMORYMANAGER_MEMORYMANAGER_INTERFACE_WEBOS_H_
#define NEVA_WEBOS_IMPL_PAL_MEMORYMANAGER_MEMORYMANAGER_INTERFACE_WEBOS_H_

#include "pal/public/interfaces/memory_manager_interface.h"

#include <list>
#include <memory>
#include <queue>

namespace lunabus {
class Handler;
class LunaClient;
}

namespace pal {

class MemoryManagerInterfaceWebOS : public MemoryManagerInterface {
 public:
  MemoryManagerInterfaceWebOS();
  ~MemoryManagerInterfaceWebOS() override;

  using MemoryStatusRequests =
      std::queue<GetMemoryStatusRespondCallback,
          std::list<GetMemoryStatusRespondCallback>>;

  void OnLevelChanged(const char* level_json);
  void OnMemoryStatusRespond(const char* status_json);

  void GetMemoryStatus(const GetMemoryStatusRespondCallback& on_done) override;
  std::unique_ptr<LevelChangedSubscription> AddCallback(
      const LevelChangedCallback& callback) override;

 private:
  void Initialize();

  std::unique_ptr<lunabus::LunaClient> lunaclient_;
  std::unique_ptr<lunabus::Handler> levelChangedHandler_;
  std::unique_ptr<lunabus::Handler> memoryStatusHandler_;

  LevelChangedCallbackList levelChangedCallbacks_;
  MemoryStatusRequests memoryStatusCallbacks_;
};

}  // namespace pal

#endif  // NEVA_WEBOS_IMPL_PAL_MEMORYMANAGER_MEMORYMANAGER_INTERFACE_WEBOS_H_
