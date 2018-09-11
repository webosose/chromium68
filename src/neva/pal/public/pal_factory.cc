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

#include "pal/public/pal_factory.h"

#include "base/command_line.h"
#include "pal/public/pal.h"
#include "remote_pal_ipc/remote_pal_ipc.h"

namespace pal {

//static
Pal* GetInstance() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  std::string process_type = command_line.GetSwitchValueASCII("type");

  if(process_type == "") {
    return Pal::GetPlatformInstance();
  } else if (process_type == "renderer") {
    return RemotePalIPC::GetRemoteInstance();
  } else {
    return nullptr;
  }
}

}  // namespace pal
