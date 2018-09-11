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

#include "components/media_capture_util/devices_dispatcher.h"

#include "base/callback.h"
#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/media_capture_devices.h"

using content::BrowserThread;

namespace media_capture_util {

namespace {

// Finds a device in |devices| that has |device_id|, or NULL if not found.
const content::MediaStreamDevice* FindDeviceWithId(
    const content::MediaStreamDevices& devices,
    const std::string& device_id) {
  content::MediaStreamDevices::const_iterator iter = devices.begin();
  for (; iter != devices.end(); ++iter) {
    if (iter->id == device_id) {
      return &(*iter);
    }
  }
  return NULL;
}

const content::MediaStreamDevice* GetAudioDevice(
    const std::string& requested_audio_device_id) {
  const content::MediaStreamDevices& audio_devices =
      content::MediaCaptureDevices::GetInstance()->GetAudioCaptureDevices();

  return FindDeviceWithId(audio_devices, requested_audio_device_id);
}

const content::MediaStreamDevice* GetVideoDevice(
    const std::string& requested_video_device_id) {
  const content::MediaStreamDevices& video_devices =
      content::MediaCaptureDevices::GetInstance()->GetVideoCaptureDevices();
  return FindDeviceWithId(video_devices, requested_video_device_id);
}

}  // namespace

DevicesDispatcher* DevicesDispatcher::GetInstance() {
  return base::Singleton<DevicesDispatcher>::get();
}

void DevicesDispatcher::ProcessMediaAccessRequest(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    bool accepts_video,
    bool accepts_audio,
    const content::MediaResponseCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  content::MediaStreamDevices devices;
  std::unique_ptr<content::MediaStreamUI> ui;

  if (request.audio_type == content::MEDIA_DEVICE_AUDIO_CAPTURE &&
      accepts_audio) {
    const content::MediaStreamDevice* device =
        GetAudioDevice(request.requested_audio_device_id);
    if (device)
      devices.push_back(*device);
  }
  if (request.video_type == content::MEDIA_DEVICE_VIDEO_CAPTURE &&
      accepts_video) {
    const content::MediaStreamDevice* device =
        GetVideoDevice(request.requested_video_device_id);
    if (device)
      devices.push_back(*device);
  }

  callback.Run(devices,
               devices.empty() ? content::MEDIA_DEVICE_NO_HARDWARE
                               : content::MEDIA_DEVICE_OK,
               std::move(ui));
}

}  // namespace media_capture_util
