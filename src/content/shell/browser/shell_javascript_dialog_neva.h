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

#ifndef CONTENT_SHELL_BROWSER_SHELL_JAVASCRIPT_DIALOG_NEVA_H_
#define CONTENT_SHELL_BROWSER_SHELL_JAVASCRIPT_DIALOG_NEVA_H_

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "content/public/browser/javascript_dialog_manager.h"

namespace content {

class ShellJavaScriptDialogManager;

namespace neva {

class ShellJavaScriptDialog {
 public:
  ShellJavaScriptDialog(
      content::ShellJavaScriptDialogManager* manager,
      gfx::NativeWindow parent_window,
      const GURL& origin_url,
      JavaScriptDialogType dialog_type,
      const base::string16& message_text,
      const base::string16& default_prompt_text,
      JavaScriptDialogManager::DialogClosedCallback callback);
  ~ShellJavaScriptDialog();

  // Called to cancel a dialog.
  void Cancel();

 private:
  content::JavaScriptDialogManager::DialogClosedCallback callback_;
  ShellJavaScriptDialogManager* manager_;

  void OnRunExternalJavaScriptDialogRespondCallback(bool success,
                                                    std::string& user_input);

  // NOTE: This must be the last member.
  base::WeakPtrFactory<ShellJavaScriptDialog> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(ShellJavaScriptDialog);
};

}  // namespace neva
}  // namespace content

#endif  // CONTENT_SHELL_BROWSER_SHELL_JAVASCRIPT_DIALOG_NEVA_H_
