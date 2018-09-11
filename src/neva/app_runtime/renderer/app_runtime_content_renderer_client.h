// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
#define NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_

#include "content/public/renderer/content_renderer_client.h"

namespace app_runtime {

class AppRuntimeContentRendererClient : public content::ContentRendererClient {
 public:
  void RenderFrameCreated(content::RenderFrame* render_frame) override;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_APP_RUNTIME_CONTENT_RENDERER_CLIENT_H_
