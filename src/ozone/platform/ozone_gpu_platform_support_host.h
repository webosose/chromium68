// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_PLATFORM_GPU_PLATFORM_SUPPORT_HOST_H_
#define OZONE_PLATFORM_GPU_PLATFORM_SUPPORT_HOST_H_

#include <vector>

#include "base/callback.h"
#include "base/observer_list.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/public/gpu_platform_support_host.h"

namespace ui {

class ChannelObserver;

class OzoneGpuPlatformSupportHost : public GpuPlatformSupportHost,
                                    public IPC::Sender {
 public:
  OzoneGpuPlatformSupportHost();
  ~OzoneGpuPlatformSupportHost() override;

  void RegisterHandler(GpuPlatformSupportHost* handler);
  void UnregisterHandler(GpuPlatformSupportHost* handler);

  void AddChannelObserver(ChannelObserver* observer);
  void RemoveChannelObserver(ChannelObserver* observer);

  bool IsConnected();

  // GpuPlatformSupportHost:
  void OnGpuProcessLaunched(
      int host_id,
      scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
      scoped_refptr<base::SingleThreadTaskRunner> send_runner,
      const base::Callback<void(IPC::Message*)>& send_callback) override;

  void OnChannelDestroyed(int host_id) override;

  void OnGpuServiceLaunched(
      scoped_refptr<base::SingleThreadTaskRunner> host_runner,
      scoped_refptr<base::SingleThreadTaskRunner> io_runner,
      GpuHostBindInterfaceCallback binder,
      GpuHostTerminateCallback terminate_callback) override;

  // IPC::Listener:
  void OnMessageReceived(const IPC::Message& message) override;

  // IPC::Sender:
  bool Send(IPC::Message* message) override;

 private:
  int host_id_ = -1;
  bool gpu_process_launched_ = false;

  scoped_refptr<base::SingleThreadTaskRunner> ui_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> send_runner_;
  base::Callback<void(IPC::Message*)> send_callback_;

  std::vector<GpuPlatformSupportHost*> handlers_;  // Not owned.
  base::ObserverList<ChannelObserver> channel_observers_;

  base::WeakPtr<OzoneGpuPlatformSupportHost> weak_ptr_;
  base::WeakPtrFactory<OzoneGpuPlatformSupportHost> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(OzoneGpuPlatformSupportHost);
};

}  // namespace ui

#endif  // UI_OZONE_GPU_DRM_GPU_PLATFORM_SUPPORT_HOST_H_
