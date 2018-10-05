// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "webos/common/webos_resource_delegate.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "content/public/common/content_switches.h"
#include "ui/base/ui_base_switches.h"

namespace webos {

base::FilePath WebosResourceDelegate::GetPathForResourcePack(
    const base::FilePath& pack_path,
    ui::ScaleFactor scale_factor) {
  NOTIMPLEMENTED();
  return pack_path;
}

base::FilePath WebosResourceDelegate::GetPathForLocalePack(
    const base::FilePath& pack_path,
    const std::string& locale) {
  NOTIMPLEMENTED();
  return pack_path;
}

gfx::Image WebosResourceDelegate::GetImageNamed(int resource_id) {
  return gfx::Image();
}

gfx::Image WebosResourceDelegate::GetNativeImageNamed(int resource_id) {
  return gfx::Image();
}

base::RefCountedStaticMemory* WebosResourceDelegate::LoadDataResourceBytes(
    int resource_id,
    ui::ScaleFactor scale_factor) {
  return nullptr;
}

bool WebosResourceDelegate::GetRawDataResource(int resource_id,
                                               ui::ScaleFactor scale_factor,
                                               base::StringPiece* value) {
  return false;
}

bool WebosResourceDelegate::GetLocalizedString(int message_id,
                                               base::string16* value) {
  return false;
}

bool WebosResourceDelegate::LoadBrowserResources() {
  base::FilePath path;
  PathService::Get(base::DIR_ASSETS, &path);
  base::FilePath resource_path =
      path.Append(FILE_PATH_LITERAL("webos_resources.pak"));
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        resource_path,
        ui::SCALE_FACTOR_100P);
    return true;
}

// static
void WebosResourceDelegate::InitializeResourceBundle() {
  WebosResourceDelegate* resource_delegate = new WebosResourceDelegate();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  const std::string locale = command_line->GetSwitchValueASCII(switches::kLang);
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, resource_delegate, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
}

}  // namespace webos
