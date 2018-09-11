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

#include "neva/app_runtime/browser/app_runtime_plugin_service_filter.h"

#include "content/public/common/webplugininfo.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"

namespace app_runtime {

bool AppRuntimePluginServiceFilter::IsPluginAvailable(
    int render_process_id,
    int render_frame_id,
    const void* context,
    const GURL& url,
    const url::Origin& main_frame_origin,
    content::WebPluginInfo* plugin) {
  NOTIMPLEMENTED();
  return false;
}

bool AppRuntimePluginServiceFilter::CanLoadPlugin(int render_process_id,
                                                  const base::FilePath& path) {
  return false;
}

}  // namespace app_runtime
