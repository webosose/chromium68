// Copyright 2019 LG Electronics, Inc.
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

#include "extensions/shell/neva/language_listener.h"

#include <string>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/json/json_reader.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/renderer_preferences.h"
#include "extensions/common/switches.h"
#include "pal/public/pal.h"

namespace {

const base::StringPiece kErrorTextKey = "errorText";
const base::StringPiece kReturnValueKey = "returnValue";

std::initializer_list<base::StringPiece> kPathToUILocaleInfoKey = {
    "settings", "localeInfo", "locales", "UI"};

}  // namespace

namespace neva {

DEFINE_WEB_CONTENTS_USER_DATA_KEY(LanguageListener);

LanguageListener::LanguageListener(content::WebContents* web_contents)
    : content::WebContentsObserver(web_contents) {
  pal::SystemLocaleInterface* interface_ =
      pal::Pal::GetPlatformInstance()->GetSystemLocaleInterface();

  if (!interface_) {
    LOG(ERROR) << __func__
               << "(): no PAL-implementation for the interface found";
    return;
  }

  // TODO(sergey.kipet@lge.com): the application id below should be:
  // 1) provided in accordance with a particular platform requirements OR
  // 2) upcasted and made common (for being used by any supported platform)
  std::string app_id;
#if defined(OS_WEBOS)
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(extensions::switches::kWebOSAppId)) {
    LOG(ERROR) << __func__ << "(): no webOS-application identifier specified";
    return;
  } else {
    app_id =
        command_line->GetSwitchValueASCII(extensions::switches::kWebOSAppId);
  }
#endif  // defined(OS_WEBOS)

  if (!interface_->Initialize(app_id.c_str())) {
    LOG(ERROR) << __func__ << "(): failed to initialize the acquired interface";
    return;
  }

  if (!localeInfoChangedCallback_)
    localeInfoChangedCallback_ = base::Bind(
        &LanguageListener::LocaleInfoUpdated, base::Unretained(this));

  if (!localeInfoChangedSubscription_.get())
    localeInfoChangedSubscription_ =
        interface_->AddCallback(localeInfoChangedCallback_);
}

LanguageListener::~LanguageListener() {
  if (interface_) {
    interface_->Shutdown();
  }
}

void LanguageListener::LocaleInfoUpdated(const std::string& locale_info_json) {
  std::unique_ptr<base::Value> root(
      base::JSONReader::Read(locale_info_json.c_str()));
  if (!root.get()) {
    LOG(ERROR) << __func__ << "(): failed to process received JSON";
    return;
  }

  const base::Value* return_value = root->FindKey(kReturnValueKey);
  if (!return_value) {
    LOG(ERROR) << __func__ << "(): failed to get result value of the operation";
    return;
  }

  // was the operation successful?
  if (return_value->GetBool()) {
    const base::Value* language = root->FindPath(kPathToUILocaleInfoKey);
    if (!language) {
      LOG(ERROR) << __func__ << "(): failed to find locale info";
      return;
    }

    content::WebContents* contents = web_contents();
    if (!contents) {
      LOG(ERROR) << __func__ << "(): failed to get WebContents instance";
      return;
    }

    auto* renderer_prefs(contents->GetMutableRendererPrefs());
    if (!renderer_prefs->accept_languages.compare(language->GetString())) {
      LOG(ERROR) << __func__
                 << "(): failed to get RendererPreferences instance";
      return;
    }
    renderer_prefs->accept_languages = language->GetString();

    content::RenderViewHost* rvh = contents->GetRenderViewHost();
    if (!rvh) {
      LOG(ERROR) << __func__ << "(): failed to get RenderViewHost instance";
      return;
    }
    rvh->SyncRendererPrefs();
  } else {
    const base::Value* error_text = root->FindKey(kErrorTextKey);
    if (error_text)
      LOG(ERROR) << __func__ << "(): error: " << error_text->GetString();
    else
      LOG(ERROR) << __func__ << "(): failed to retrieve error message";
  }
}

}  // namespace neva
