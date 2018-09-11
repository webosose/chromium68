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

#ifndef NEVA_WAM_DEMO_WAM_DEMO_WEBAPP_H_
#define NEVA_WAM_DEMO_WAM_DEMO_WEBAPP_H_

#include "content/public/common/main_function_params.h"
#include "neva/app_runtime/public/injection_browser_control_handler.h"
#include "neva/app_runtime/public/webapp_window_base.h"
#include "neva/app_runtime/public/webview_base.h"

namespace wam_demo {

class BlinkViewObserver {
 public:
  virtual void OnDocumentLoadFinished(app_runtime::WebViewBase* view) = 0;
  virtual void OnLoadFailed(app_runtime::WebViewBase* view) = 0;
  virtual void OnRenderProcessGone(app_runtime::WebViewBase* view) = 0;
  virtual void OnRenderProcessCreated(app_runtime::WebViewBase* view) = 0;
  virtual void OnTitleChanged(app_runtime::WebViewBase* view,
                              const std::string& title) = 0;
  virtual void OnBrowserControlCommand(
      const std::string& name,
      const std::vector<std::string>& args) = 0;

  virtual std::string OnBrowserControlFunction(
      const std::string& name,
      const std::vector<std::string>& args) = 0;
 };

class BlinkView : public app_runtime::WebViewBase,
                  public app_runtime::InjectionBrowserControlHandlerDelegate {
 public:
  BlinkView(BlinkViewObserver* observer);
  BlinkView(int width, int height, BlinkViewObserver* observer);
  ~BlinkView() override;

  // from WebViewDelegate
  void OnLoadProgressChanged(double progress) override;
  void DidFirstFrameFocused() override;
  void DidFirstNonBlankPaint() override;
  void LoadVisuallyCommitted() override;
  void TitleChanged(const std::string& title) override;
  void NavigationHistoryChanged() override;
  void Close() override;
  bool DecidePolicyForResponse(bool is_main_frame,
                               int status_code,
                               const std::string& url,
                               const std::string& status_text) override;

  // Additional methods for testing
  void SetMediaCapturePermission();
  void ClearMediaCapturePermission();

  bool AcceptsVideoCapture() override;
  bool AcceptsAudioCapture() override;
  void LoadStarted() override;
  void LoadFinished(const std::string& url) override;
  void LoadFailed(const std::string& url,
                  int error_code,
                  const std::string& error_description) override;
  void LoadStopped(const std::string& url) override;
  void RenderProcessCreated(int pid) override;
  void RenderProcessGone() override;
  void DocumentLoadFinished() override;
  void DidHistoryBackOnTopPage() override;
  void DidClearWindowObject() override;
  void SetDecidePolicyForResponse();

  // InjectionBrowserControlHandlerDelegate
  void OnBrowserControlCommand(
      const std::string& command,
      const std::vector<std::string>& arguments) override;
  void OnBrowserControlFunction(
      const std::string& command,
      const std::vector<std::string>& arguments,
      std::string* result) override;
private:
  BlinkViewObserver* observer_;
  std::string title_;
  std::string progress_;
  bool accepts_video_capture_;
  bool accepts_audio_capture_;
  static bool policy_;
  std::unique_ptr<app_runtime::InjectionBrowserControlHandler>
      browser_control_handler_;
};

class WebAppWindowImpl;

class WebAppWindowImplObserver {
public:
  virtual void OnWindowClosing(WebAppWindowImpl* window) = 0;
};

class WebAppWindowImpl : public app_runtime::WebAppWindowBase {
 public:
  WebAppWindowImpl(const app_runtime::WebAppWindowBase::CreateParams& params,
                   WebAppWindowImplObserver* observer)
      : app_runtime::WebAppWindowBase(params),
        observer_(observer) {}

  void OnWindowClosing() override;

 private:
  WebAppWindowImplObserver* observer_;
};

struct WamDemoApplication {
  WamDemoApplication(const std::string& appid,
                     const std::string& url,
                     WebAppWindowImpl* win,
                     BlinkView* page)
      : appid_(appid), url_(url), win_(win), page_(page) {}

  std::string appid_;
  std::string url_;
  WebAppWindowImpl* win_ = nullptr;
  BlinkView* page_ = nullptr;
  bool render_gone_ = false;
};

}  // namespace wam_demo

#endif  // NEVA_WAM_DEMO_WAM_DEMO_WEBAPP_H_
