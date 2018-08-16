// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_MANAGER_H_
#define OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_MANAGER_H_

#include <list>
#include <string>

#include "base/memory/weak_ptr.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace ui {

class InputMethodContextImplWayland;
class OzoneGpuPlatformSupportHost;

class InputMethodContextManager : public GpuPlatformSupportHost {
 public:
  InputMethodContextManager(OzoneGpuPlatformSupportHost* sender);
  ~InputMethodContextManager() override;

  void AddContext(InputMethodContextImplWayland* context);
  void RemoveContext(InputMethodContextImplWayland* context);

  void ImeReset();
 private:
  // GpuPlatformSupportHost
  void OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) override;
  void OnGpuServiceLaunched(
      scoped_refptr<base::SingleThreadTaskRunner> host_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuHostBindInterfaceCallback binder,
      GpuHostTerminateCallback terminate_callback) override;
  void OnChannelDestroyed(int host_id) override;
  void OnMessageReceived(const IPC::Message&) override;

  void NotifyCommit(unsigned handle, const std::string& text);
  void NotifyPreeditChanged(unsigned handle,
                        const std::string& text,
                        const std::string& commit);
  void NotifyPreeditEnd();
  void NotifyPreeditStart();
  void NotifyDeleteRange(unsigned handle, int32_t index, uint32_t length);
  void Commit(unsigned handle, const std::string& text);
  void PreeditChanged(unsigned handle,
                      const std::string& text,
                      const std::string& commit);
  void PreeditEnd();
  void PreeditStart();
  void DeleteRange(unsigned handle, int32_t index, uint32_t length);

  InputMethodContextImplWayland* GetContext(unsigned handle);

  std::list<InputMethodContextImplWayland*> contexts_list_;
  OzoneGpuPlatformSupportHost* sender_;
  base::WeakPtrFactory<InputMethodContextManager> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodContextManager);
};

}  // namespace ui

#endif  //  OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_MANAGER_H_
