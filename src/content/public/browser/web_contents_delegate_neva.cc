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

#include "content/public/browser/web_contents_delegate_neva.h"

#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "content/public/browser/color_chooser_neva.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/common/file_chooser_file_info.h"
#include "content/public/common/file_chooser_params.h"
#include "pal/ipc/pal_macros.h"
#include "pal/public/pal.h"

namespace content {
namespace neva {

WebContentsDelegate::WebContentsDelegate() : weak_ptr_factory_(this) {}
WebContentsDelegate::~WebContentsDelegate() {}

content::ColorChooser* WebContentsDelegate::OpenColorChooser(
    WebContents* web_contents,
    SkColor color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseExternalInputControls)) {
      return ::neva::ColorChooser::Open(web_contents, color, suggestions);
  } else {
    return content::WebContentsDelegate::OpenColorChooser(web_contents, color,
                                                          suggestions);
  }
}

void WebContentsDelegate::RunFileChooser(RenderFrameHost* render_frame_host,
                                         const FileChooserParams& params) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kUseExternalInputControls)) {
    pal::NativeControlsInterface* interface =
        pal::Pal::GetPlatformInstance()->GetNativeControlsInterface();
    if (interface) {
      std::string title(base::UTF16ToUTF8(params.title));
      std::string default_file_name(params.default_file_name.AsUTF8Unsafe());
      std::vector<std::string> accept_types;
      for (auto type : params.accept_types)
        accept_types.push_back(base::UTF16ToUTF8(type));

      interface->RunFileChooser(
          params.mode, title, default_file_name, accept_types,
          params.need_local_path, params.requestor.spec(),
          base::Bind(&WebContentsDelegate::OnRunFileChooserRespondCallback,
                     weak_ptr_factory_.GetWeakPtr(), render_frame_host));
    } else {
      LOG(ERROR) << "Interface not available";
    }
  } else {
    content::WebContentsDelegate::RunFileChooser(render_frame_host, params);
  }
}

void WebContentsDelegate::OnRunFileChooserRespondCallback(
    RenderFrameHost* render_frame_host, std::string selected_files) {
  std::string error_msg;
  std::unique_ptr<base::Value> response = base::JSONReader::ReadAndReturnError(
      selected_files, base::JSON_PARSE_RFC, NULL, &error_msg);
  if (response == NULL) {
    LOG(ERROR) << __func__ << "() : JSONReader failed : " << error_msg;
    return;
  }
  if (!response->is_dict()) {
    LOG(ERROR) << __func__ << "() : Unexpected response type "
               << response->type();
    return;
  }

  const base::DictionaryValue* response_object =
      static_cast<base::DictionaryValue*>(response.get());
  const base::ListValue* selected_files_value;
  int mode_value;
  FileChooserParams::Mode mode;

  if (!response_object->GetInteger("mode", &mode_value)) {
    LOG(ERROR) << __func__ << "() : Absent argument 'mode'";
    return;
  }

  mode = static_cast<FileChooserParams::Mode>(mode_value);

  if (!response_object->GetList("file_list", &selected_files_value)) {
    LOG(ERROR) << __func__ << "() : Absent argument 'file_list'";
    return;
  }

  const base::DictionaryValue* selected_file_dict;
  std::vector<content::FileChooserFileInfo> chooser_files;
  for (size_t i = 0; i < selected_files_value->GetSize(); i++) {
    if (!selected_files_value->GetDictionary(i, &selected_file_dict)) {
      LOG(ERROR) << __func__
                 << "() : Unexpected type of response field 'file_list'";
      return;
    }

    content::FileChooserFileInfo chooser_file;
    std::string path;
    bool success = selected_file_dict->GetString("file_path", &path);
    success = success &&
              selected_file_dict->GetString("display_name",
                                            &chooser_file.display_name);
    int size = 0;
    success = success && selected_file_dict->GetInteger("size", &size);
    std::string modification_time_value;
    success = success &&
              selected_file_dict->GetString("modification_time",
                                            &modification_time_value);

    if (!success) {
      LOG(ERROR) << __func__ << "() : Incorrect arguments";
      return;
    }
    chooser_file.file_path = base::FilePath(path);
    chooser_file.length = size;
    chooser_file.modification_time =
        base::Time::FromJavaTime(std::stoull(modification_time_value));
    chooser_file.is_directory = mode == FileChooserParams::UploadFolder;
    chooser_files.push_back(chooser_file);
  }
  render_frame_host->FilesSelectedInChooser(chooser_files, mode);
}

}  // namespace neva
}  // namespace content
