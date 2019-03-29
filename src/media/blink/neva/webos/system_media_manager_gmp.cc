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

#include "media/blink/neva/webos/system_media_manager_gmp.h"

#include <algorithm>
#include <uMediaClient.h>

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "media/blink/neva/webos/media_util.h"
#include "media/blink/neva/webos/umediaclient_impl.h"
#include "third_party/jsoncpp/source/include/json/json.h"

#define FUNC_LOG(x) DVLOG(x) << __func__

namespace media {
std::unique_ptr<SystemMediaManager> SystemMediaManager::Create(
    const base::WeakPtr<UMediaClientImpl>& umedia_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner) {
  return std::make_unique<SystemMediaManagerGmp>(umedia_client,
                                                 main_task_runner);
}

SystemMediaManagerGmp::SystemMediaManagerGmp(
    const base::WeakPtr<UMediaClientImpl>& umedia_client,
    const scoped_refptr<base::SingleThreadTaskRunner>& main_task_runner)
    : umedia_client_(umedia_client),
      main_task_runner_(main_task_runner),
      weak_factory_(this) {
  FUNC_LOG(1) << " umedia_client_=" << umedia_client_.get();
}

SystemMediaManagerGmp::~SystemMediaManagerGmp() {}

bool SystemMediaManagerGmp::SetDisplayWindow(const gfx::Rect& out_rect,
                                             const gfx::Rect& in_rect,
                                             bool fullscreen) {
  if (fullscreen) {
    LOG(INFO) << __func__ << " fullscreen=" << fullscreen;
    umedia_client_->switchToFullscreen();
  } else if (in_rect.IsEmpty()) {
    LOG(INFO) << __func__ << " - outRect: " << out_rect.ToString()
              << ", fullscreen: " << fullscreen;
    return umedia_client_->setDisplayWindow(uMediaServer::rect_t(
        out_rect.x(), out_rect.y(), out_rect.width(), out_rect.height()));
  } else {
    LOG(INFO) << __func__ << " - inRect: " << in_rect.ToString()
              << ", outRect: " << out_rect.ToString()
              << ", fullscreen: " << fullscreen;
    return umedia_client_->setDisplayWindow(
        uMediaServer::rect_t(in_rect.x(), in_rect.y(), in_rect.width(),
                             in_rect.height()),
        uMediaServer::rect_t(out_rect.x(), out_rect.y(), out_rect.width(),
                             out_rect.height()));
  }
  return true;
}

void SystemMediaManagerGmp::SetVisibility(bool visible) {
  if (visibility_ == visible)
    return;
  if (!visible)
    SetDisplayWindow(gfx::Rect(0, 0, 1, 1), gfx::Rect(0, 0, 1, 1), false);
  visibility_ = visible;
}

bool SystemMediaManagerGmp::GetVisibility() {
  return visibility_;
}

}  // namespace media
