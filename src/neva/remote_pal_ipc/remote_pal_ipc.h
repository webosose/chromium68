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

#ifndef REMOTE_PAL_IPC_REMOTE_PAL_IPC_H_
#define REMOTE_PAL_IPC_REMOTE_PAL_IPC_H_

#include <memory>
#include "pal/public/pal.h"

#if defined(COMPONENT_BUILD)
#if defined(REMOTE_PAL_IMPLEMENTATION)
#define REMOTE_PAL_EXPORT __attribute__((visibility("default")))
#else
#define REMOTE_PAL_EXPORT
#endif

#else // defined(COMPONENT_BUILD)
#define REMOTE_PAL_EXPORT
#endif

namespace pal {

class REMOTE_PAL_EXPORT RemotePalIPC : public Pal {
 public:
  RemotePalIPC();
  ~RemotePalIPC() override;

  static Pal* GetRemoteInstance();

  #include "remote_pal_ipc/remote_pal_ipc_gen.h"
};

}  // namespace pal

#endif  // REMOTE_PAL_IPC_REMOTE_PAL_IPC_H_
