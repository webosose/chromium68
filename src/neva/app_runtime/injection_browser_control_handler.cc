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

#include "neva/app_runtime/public/injection_browser_control_handler.h"

#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "neva/app_runtime/public/webview_base_internals.h"
#include "neva/neva_chromium/content/common/browser_control_messages.h"
#include "ipc/ipc_sender.h"

namespace app_runtime {

class InjectionBrowserControlHandler::InjectionBrowserControlObserver
    : public content::WebContentsObserver,
      public IPC::Sender {
 public:
  InjectionBrowserControlObserver(
      internals::WebViewBaseInternals* webview_base_internals)
      : webview_base_internals_(webview_base_internals) {
      Observe(webview_base_internals_->GetWebContents());
  }

  // IPC::Listener implementation.
  bool OnMessageReceived(const IPC::Message& message) override {
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(InjectionBrowserControlObserver, message)
      IPC_MESSAGE_HANDLER(BrowserControlMsg_Command, OnBrowserControlCommand)
      IPC_MESSAGE_HANDLER(BrowserControlMsg_Function, OnBrowserControlFunction)
      IPC_MESSAGE_UNHANDLED(handled = false)
    IPC_END_MESSAGE_MAP()
    return handled;
  }

  // IPC::Sender implementation.
  bool Send(IPC::Message* msg) override {
    if (webview_base_internals_->GetWebContents()) {
      content::RenderViewHost* rvh =
          webview_base_internals_->GetWebContents()->GetRenderViewHost();
      if (rvh)
        return rvh->Send(msg);
    }
    delete msg;
    return false;
  }

   void SetDelegate(InjectionBrowserControlHandlerDelegate* delegate) {
    delegate_ = delegate;
  }

  InjectionBrowserControlHandlerDelegate* GetDelegate() const {
    return delegate_;
  }

 private:
  void OnBrowserControlCommand(const std::string& command,
                               const std::vector<std::string>& arguments) {
    if (delegate_)
      delegate_->OnBrowserControlCommand(command, arguments);
  }

  void OnBrowserControlFunction(const std::string& command,
                                const std::vector<std::string>& arguments,
                                std::string* result) {
    if (delegate_)
      delegate_->OnBrowserControlFunction(command, arguments, result);
  }

  // WebViewBase* web_view_base_ = nullptr;
  internals::WebViewBaseInternals* webview_base_internals_ = nullptr;
  InjectionBrowserControlHandlerDelegate* delegate_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(InjectionBrowserControlObserver);
};

InjectionBrowserControlHandler::InjectionBrowserControlHandler(
    internals::WebViewBaseInternals* webview_base_internals)
    : injection_observer_(
        new InjectionBrowserControlObserver(webview_base_internals)) {
}

InjectionBrowserControlHandler::~InjectionBrowserControlHandler() {
}

void InjectionBrowserControlHandler::SetDelegate(
    InjectionBrowserControlHandlerDelegate* delegate) {
  injection_observer_->SetDelegate(delegate);
}

InjectionBrowserControlHandlerDelegate*
    InjectionBrowserControlHandler::GetDelegate() const {
  return injection_observer_->GetDelegate();
}

}  // appruntime

