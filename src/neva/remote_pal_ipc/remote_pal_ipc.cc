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

#include "remote_pal_ipc/remote_pal_ipc.h"

#include <mutex>

namespace pal {

RemotePalIPC::RemotePalIPC() = default;
RemotePalIPC::~RemotePalIPC() = default;

std::unique_ptr<Pal> RemotePalInstance;
std::once_flag remote_pal_pc_instance_flag;

//static
Pal* RemotePalIPC::GetRemoteInstance() {
  std::call_once(remote_pal_pc_instance_flag,
                 []() { RemotePalInstance.reset(new RemotePalIPC()); });
  return RemotePalInstance.get();
}

}  // namespace pal
