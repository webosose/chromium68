// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_GMP_H_
#define MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_GMP_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "media/blink/neva/webos/system_media_manager.h"
#include "media/blink/neva/webos/webos_mediaclient.h"

namespace gfx {
class Rect;
}

namespace Json {
class Value;
}

namespace ums {
struct audio_info_t;
struct video_info_t;
}

namespace media {
class UMediaClientImpl;

class SystemMediaManagerGmp : public SystemMediaManager {
 public:
  SystemMediaManagerGmp(
      const base::WeakPtr<UMediaClientImpl>& umedia_client,
      const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner);
  ~SystemMediaManagerGmp() override;

  long Initialize(const bool is_video,
                  const std::string& app_id,
                  const ActiveRegionCB& active_region_cb) override {
    return 0;
  }
  void UpdateHtmlMediaOption(const Json::Value& option) override {}
  bool SetDisplayWindow(const gfx::Rect& out_rect,
                        const gfx::Rect& in_rect,
                        bool fullscreen) override;
  void SetVisibility(bool visible) override;
  bool GetVisibility() override;
  void SetAudioFocus() override {}
  bool GetAudioFocus() override { return true; }
  void SwitchToAutoLayout() override {}
  void AudioInfoUpdated(const struct ums::audio_info_t& audio_info) override {}
  void VideoInfoUpdated(const struct ums::video_info_t& videoInfo) override {}
  void SourceInfoUpdated(bool has_video, bool has_audio) override {}
  void AppStateChanged(AppState s) override {}
  void PlayStateChanged(PlayState s) override {}
  void AudioMuteChanged(bool mute) override {}
  bool SendCustomMessage(const std::string& message) override { return true; }
  void EofReceived() override {}

 private:
  bool visibility_ = false;

  base::WeakPtr<UMediaClientImpl> umedia_client_;
  const scoped_refptr<base::SingleThreadTaskRunner> main_task_runner_;

  // Member variables should appear before the WeakPtrFactory, to ensure
  // that any WeakPtrs to Controller are invalidated before its members
  // variable's destructors are executed, rendering them invalid.
  base::WeakPtrFactory<SystemMediaManagerGmp> weak_factory_;
};

}  // namespace media
#endif  // MEDIA_BLINK_NEVA_WEBOS_SYSTEM_MEDIA_MANAGER_GMP_H_
