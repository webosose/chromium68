// Copyright 2018 LG Electronics, Inc.
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

#ifndef WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_
#define WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_

#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"

namespace webos {

class WebOSContentRendererClient
    : public app_runtime::AppRuntimeContentRendererClient {
 public:
  void AddSupportedKeySystems(
      std::vector<std::unique_ptr<media::KeySystemProperties>>* key_systems)
      override;
};

}  // namespace webos

#endif  // WEBOS_RENDERER_WEBOS_CONTENT_RENDERER_CLIENT_H_
