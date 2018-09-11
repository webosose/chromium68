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

#ifndef REMOTE_PAL_IPC_REMOTE_PAL_OBSERVER_H_
#define REMOTE_PAL_IPC_REMOTE_PAL_OBSERVER_H_

#include "content/public/renderer/render_view_observer.h"

namespace content {

class RemotePalObserver : public RenderViewObserver {
 public:
  explicit RemotePalObserver(RenderViewImpl* render_view);
  ~RemotePalObserver() override;

  // RenderViewObserver overrides:
  bool OnMessageReceived(const IPC::Message& message) override;
  void OnDestruct() override;

 private:
  void InitializeInterfaceObservers();

  DISALLOW_COPY_AND_ASSIGN(RemotePalObserver);
};

}  // namespace content

#endif  // REMOTE_PAL_IPC_REMOTE_PAL_OBSERVER_H_
