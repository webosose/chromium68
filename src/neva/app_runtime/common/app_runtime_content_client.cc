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

#include "neva/app_runtime/common/app_runtime_content_client.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/pepper_plugin_info.h"
#include "content/public/common/user_agent.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ui/base/resource/resource_bundle.h"

namespace app_runtime {

AppRuntimeContentClient::~AppRuntimeContentClient() {
}

base::StringPiece AppRuntimeContentClient::GetDataResource(
    int resource_id,
    ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* AppRuntimeContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

gfx::Image& AppRuntimeContentClient::GetNativeImageNamed(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

std::string AppRuntimeContentClient::GetUserAgent() const {
  return app_runtime::GetUserAgent();
}

void AppRuntimeContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  NOTIMPLEMENTED();
}

}  // namespace app_runtime
