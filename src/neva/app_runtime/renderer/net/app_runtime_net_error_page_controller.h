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

#ifndef NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_PAGE_CONTROLLER_H_
#define NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_PAGE_CONTROLLER_H_

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "chrome/renderer/net/net_error_helper_core.h"
#include "gin/arguments.h"
#include "gin/wrappable.h"

namespace content {
class RenderFrame;
}

// This class makes various helper functions available to the
// error page loaded by NetErrorHelper.  It is bound to the JavaScript
// window.errorPageController object.
class AppRuntimeNetErrorPageController
    : public gin::Wrappable<AppRuntimeNetErrorPageController> {
 public:
  enum Button { SETTINGS_BUTTON };

  static gin::WrapperInfo kWrapperInfo;

  // Interface used to notify creator of user actions invoked on the
  // error page.
  class Delegate {
   public:
    // Button press notification from error page.
    virtual void ButtonPressed(AppRuntimeNetErrorPageController::Button button,
                               int target_id) = 0;

   protected:
    Delegate();
    virtual ~Delegate();

    DISALLOW_COPY_AND_ASSIGN(Delegate);
  };

  // Will invoke methods on |delegate| in response to user actions taken on the
  // error page. May call delegate methods even after the page has been
  // navigated away from, so it is recommended consumers make sure the weak
  // pointers are destroyed in response to navigations.
  static void Install(content::RenderFrame* render_frame,
                      base::WeakPtr<Delegate> delegate);

 private:
  explicit AppRuntimeNetErrorPageController(base::WeakPtr<Delegate> delegate);
  ~AppRuntimeNetErrorPageController() override;

  // Execute a "Reload" button click.
  bool SettingsButtonClick(const gin::Arguments& args);

  // Used internally by other button click methods.
  bool ButtonClick(AppRuntimeNetErrorPageController::Button button,
                   const gin::Arguments& args);

  // gin::WrappableBase
  gin::ObjectTemplateBuilder GetObjectTemplateBuilder(
      v8::Isolate* isolate) override;

  base::WeakPtr<Delegate> delegate_;

  DISALLOW_COPY_AND_ASSIGN(AppRuntimeNetErrorPageController);
};

#endif  // NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_PAGE_CONTROLLER_H_
