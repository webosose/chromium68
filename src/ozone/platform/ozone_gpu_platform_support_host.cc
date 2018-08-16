// Copyright 2014 The Chromium Authors. All rights reserved.
// Copyright 2015 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/platform/channel_observer.h"
#include "ozone/platform/ozone_gpu_platform_support_host.h"

#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/trace_event.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/common/gpu/ozone_gpu_messages.h"

namespace ui {

OzoneGpuPlatformSupportHost::OzoneGpuPlatformSupportHost()
    : ui_runner_(base::ThreadTaskRunnerHandle::IsSet()
                     ? base::ThreadTaskRunnerHandle::Get()
                     : nullptr),
      weak_ptr_factory_(this) {
  if (ui_runner_) {
    weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
  }
}

OzoneGpuPlatformSupportHost::~OzoneGpuPlatformSupportHost() {
}

void OzoneGpuPlatformSupportHost::RegisterHandler(
    GpuPlatformSupportHost* handler) {
  handlers_.push_back(handler);

  if (IsConnected())
    handler->OnGpuProcessLaunched(host_id_, ui_runner_, send_runner_, send_callback_);
}

void OzoneGpuPlatformSupportHost::UnregisterHandler(
    GpuPlatformSupportHost* handler) {
  std::vector<GpuPlatformSupportHost*>::iterator it =
      std::find(handlers_.begin(), handlers_.end(), handler);
  if (it != handlers_.end())
    handlers_.erase(it);
}

void OzoneGpuPlatformSupportHost::AddChannelObserver(
    ChannelObserver* observer) {
  channel_observers_.AddObserver(observer);

  if (IsConnected())
    observer->OnGpuProcessLaunched();
}

void OzoneGpuPlatformSupportHost::RemoveChannelObserver(
    ChannelObserver* observer) {
  channel_observers_.RemoveObserver(observer);
}

bool OzoneGpuPlatformSupportHost::IsConnected() {
  return host_id_ >= 0 && gpu_process_launched_;
}

void OzoneGpuPlatformSupportHost::OnGpuProcessLaunched(
    int host_id,
    scoped_refptr<base::SingleThreadTaskRunner> ui_runner,
    scoped_refptr<base::SingleThreadTaskRunner> send_runner,
    const base::Callback<void(IPC::Message*)>& send_callback) {
  // If there was a task runner set during construction, prefer using that.
  if (!ui_runner_) {
    ui_runner_ = std::move(ui_runner);
    weak_ptr_ = weak_ptr_factory_.GetWeakPtr();
  }
  TRACE_EVENT1("drm", "OzoneGpuPlatformSupportHost::OnGpuProcessLaunched",
               "host_id", host_id);
  gpu_process_launched_ = true;
  host_id_ = host_id;
  send_runner_ = std::move(send_runner);
  send_callback_ = send_callback;

  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->OnGpuProcessLaunched(host_id, ui_runner_, send_runner_, send_callback_);

  for (auto& observer : channel_observers_)
    observer.OnGpuProcessLaunched();
}

void OzoneGpuPlatformSupportHost::OnChannelDestroyed(int host_id) {
  TRACE_EVENT1("drm", "OzoneGpuPlatformSupportHost::OnChannelDestroyed",
               "host_id", host_id);
  gpu_process_launched_ = false;
  if (host_id_ == host_id) {
    host_id_ = -1;
    send_runner_ = nullptr;
    send_callback_.Reset();
    for (auto& observer : channel_observers_)
      observer.OnChannelDestroyed();
  }

  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->OnChannelDestroyed(host_id);
}

void OzoneGpuPlatformSupportHost::OnGpuServiceLaunched(
    scoped_refptr<base::SingleThreadTaskRunner> host_runner,
    scoped_refptr<base::SingleThreadTaskRunner> io_runner,
    GpuHostBindInterfaceCallback binder,
    GpuHostTerminateCallback terminate_callback) {
  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->OnGpuServiceLaunched(host_runner, io_runner, binder,
                                       std::move(terminate_callback));
}

void OzoneGpuPlatformSupportHost::OnMessageReceived(
    const IPC::Message& message) {
  if (ui_runner_->BelongsToCurrentThread()) {
    for (size_t i = 0; i < handlers_.size(); ++i)
      handlers_[i]->OnMessageReceived(message);
  } else {
    ui_runner_->PostTask(
        FROM_HERE, base::BindOnce(&OzoneGpuPlatformSupportHost::OnMessageReceived,
                                  weak_ptr_, message));
  }
}

bool OzoneGpuPlatformSupportHost::Send(IPC::Message* message) {
  if (IsConnected() &&
      send_runner_->PostTask(FROM_HERE, base::Bind(send_callback_, message)))
    return true;

  delete message;
  return false;
}

}  // namespace ui
