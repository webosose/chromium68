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

#include "content/shell/browser/shell_javascript_dialog_neva.h"

#include "base/strings/utf_string_conversions.h"
#include "components/url_formatter/url_formatter.h"
#include "content/shell/browser/shell_javascript_dialog_manager.h"
#include "pal/ipc/pal_macros.h"
#include "pal/public/pal.h"

namespace content {
namespace neva {

ShellJavaScriptDialog::ShellJavaScriptDialog(
    content::ShellJavaScriptDialogManager* manager,
    gfx::NativeWindow parent_window, const GURL& origin_url,
    JavaScriptDialogType dialog_type, const base::string16& message,
    const base::string16& default_prompt,
    JavaScriptDialogManager::DialogClosedCallback callback)
    : callback_(std::move(callback)), manager_(manager), weak_ptr_factory_(this) {
  pal::NativeDialogsInterface* interface =
      pal::Pal::GetPlatformInstance()->GetNativeDialogsInterface();

  if (interface) {
    std::string message_txt(base::UTF16ToUTF8(message));
    std::string default_prompt_txt(base::UTF16ToUTF8(default_prompt));
    interface->RunJavaScriptDialog(
        dialog_type, message_txt, default_prompt_txt, origin_url.spec(),
        base::Bind(&ShellJavaScriptDialog::
                       OnRunExternalJavaScriptDialogRespondCallback,
                   weak_ptr_factory_.GetWeakPtr()));
  } else {
    LOG(ERROR) << "Interface not available";
  }
  return;
}

void ShellJavaScriptDialog::OnRunExternalJavaScriptDialogRespondCallback(
    bool success, std::string& user_input) {
  if (!callback_.is_null())
    std::move(callback_).Run(success, base::UTF8ToUTF16(user_input));
  manager_->DialogClosed(nullptr);
}

ShellJavaScriptDialog::~ShellJavaScriptDialog() { Cancel(); }

void ShellJavaScriptDialog::Cancel() {
  pal::NativeDialogsInterface* interface =
      pal::Pal::GetPlatformInstance()->GetNativeDialogsInterface();
  if (interface) {
    interface->CancelJavaScriptDialog();
  } else {
    LOG(ERROR) << "Interface not available";
  }
  return;
}

}  // namespace neva
}  // namespace content
