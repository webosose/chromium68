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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_NEVA_WEB_SETTINGS_IMPL_NEVA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_NEVA_WEB_SETTINGS_IMPL_NEVA_H_

#include "third_party/blink/public/web/web_settings.h"
#include "third_party/blink/renderer/core/frame/settings.h"

namespace blink {

class WebSettingsImplNeva : public WebSettings {
 public:
  explicit WebSettingsImplNeva(Settings* settings) : settings_(settings) {}

  void SetNotifyFMPDirectly(bool directly) override {
    settings_->SetNotifyFMPDirectly(directly);
  }

 private:
  Settings* settings_;
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_EXPORTED_NEVA_WEB_SETTINGS_IMPL_NEVA_H_
