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

#include "injection/common/public/renderer/injection_browser_control_base.h"

#include <string>

#include "content/public/renderer/render_view.h"
#include "injection/common/public/renderer/injection_base.h"
#include "neva_chromium/content/common/browser_control_messages.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "third_party/blink/public/web/web_script_source.h"

namespace extensions_v8 {

InjectionBrowserControlBase::InjectionBrowserControlBase() {}

void InjectionBrowserControlBase::SendCommand(const std::string& function) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "command=" << function;
#endif
  std::vector<std::string> arguments;
  SendCommand(function, arguments);
}

void InjectionBrowserControlBase::SendCommand(
    const std::string& function,
    const std::vector<std::string>& arguments) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "command=" << function;
#endif
  blink::WebFrame* frame = blink::WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return;
  content::RenderView* view = content::RenderView::FromWebView(frame->View());
  if (!view)
    return;
  view->Send(
      new BrowserControlMsg_Command(view->GetRoutingID(), function, arguments));
}

std::string InjectionBrowserControlBase::CallFunction(
    const std::string& function) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "function=" << function;
#endif
  std::vector<std::string> arguments;
  return CallFunction(function, arguments);
}

std::string InjectionBrowserControlBase::CallFunction(
    const std::string& function,
    const std::vector<std::string>& arguments) {
#ifdef USE_BASE_LOGGING
  LOG(INFO) << "function=" << function;
#endif
  std::string result;
  blink::WebFrame* frame = blink::WebLocalFrame::FrameForCurrentContext();
  if (!frame)
    return result;
  content::RenderView* view = content::RenderView::FromWebView(frame->View());
  if (!view)
    return result;

  view->Send(new BrowserControlMsg_Function(view->GetRoutingID(), function,
                                            arguments, &result));
  return result;
}

}  // namespace extensions_v8
