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

#include "neva/app_runtime/renderer/net/app_runtime_net_error_page_controller.h"

#include "base/strings/string_piece.h"
#include "content/public/renderer/render_frame.h"
#include "gin/handle.h"
#include "gin/object_template_builder.h"
#include "third_party/blink/public/web/blink.h"
#include "third_party/blink/public/web/web_local_frame.h"

gin::WrapperInfo AppRuntimeNetErrorPageController::kWrapperInfo = {
    gin::kEmbedderNativeGin};

AppRuntimeNetErrorPageController::Delegate::Delegate() {}
AppRuntimeNetErrorPageController::Delegate::~Delegate() {}

// static
void AppRuntimeNetErrorPageController::Install(
    content::RenderFrame* render_frame,
    base::WeakPtr<Delegate> delegate) {
  v8::Isolate* isolate = blink::MainThreadIsolate();
  v8::HandleScope handle_scope(isolate);
  v8::Local<v8::Context> context =
      render_frame->GetWebFrame()->MainWorldScriptContext();
  if (context.IsEmpty())
    return;

  v8::Context::Scope context_scope(context);

  gin::Handle<AppRuntimeNetErrorPageController> controller = gin::CreateHandle(
      isolate, new AppRuntimeNetErrorPageController(delegate));
  if (controller.IsEmpty())
    return;

  v8::Local<v8::Object> global = context->Global();
  global->Set(gin::StringToV8(isolate, "errorPageController"),
              controller.ToV8());
}

bool AppRuntimeNetErrorPageController::SettingsButtonClick(
    const gin::Arguments& args) {
  return ButtonClick(AppRuntimeNetErrorPageController::Button::SETTINGS_BUTTON,
                     args);
}

bool AppRuntimeNetErrorPageController::ButtonClick(
    AppRuntimeNetErrorPageController::Button button,
    const gin::Arguments& args) {
  if (args.PeekNext().IsEmpty() && !args.PeekNext()->IsInt32())
    return false;

  if (delegate_)
    delegate_->ButtonPressed(button, args.PeekNext()->Int32Value());

  return true;
}

AppRuntimeNetErrorPageController::AppRuntimeNetErrorPageController(
    base::WeakPtr<Delegate> delegate)
    : delegate_(delegate) {}

AppRuntimeNetErrorPageController::~AppRuntimeNetErrorPageController() {}

gin::ObjectTemplateBuilder
AppRuntimeNetErrorPageController::GetObjectTemplateBuilder(
    v8::Isolate* isolate) {
  return gin::Wrappable<AppRuntimeNetErrorPageController>::
      GetObjectTemplateBuilder(isolate)
          .SetMethod("settingsButtonClick",
                     &AppRuntimeNetErrorPageController::SettingsButtonClick);
}
