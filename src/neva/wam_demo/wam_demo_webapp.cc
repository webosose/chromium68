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

#include "neva/wam_demo/wam_demo_webapp.h"

#include "base/strings/string_number_conversions.h"

namespace wam_demo {

bool BlinkView::policy_ = false;

BlinkView::BlinkView(BlinkViewObserver* observer)
    : observer_(observer),
      accepts_video_capture_(false),
      accepts_audio_capture_(false),
      browser_control_handler_(
        new app_runtime::InjectionBrowserControlHandler(this)) {
  browser_control_handler_->SetDelegate(this);
}

BlinkView::BlinkView(int width, int height, BlinkViewObserver* observer)
    : WebViewBase(width, height),
      observer_(observer),
      accepts_video_capture_(false),
      accepts_audio_capture_(false),
      browser_control_handler_(
        new app_runtime::InjectionBrowserControlHandler(this)) {
  browser_control_handler_->SetDelegate(this);
}

BlinkView::~BlinkView() {}

void BlinkView::OnLoadProgressChanged(double progress) {
  progress_ = base::IntToString(progress * 100);

  std::string progress_title = title_;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);
}

void BlinkView::DidFirstFrameFocused() {
  LOG(INFO) << __func__ << "(): Did frame focused is delivered";
}

void BlinkView::DidFirstNonBlankPaint() {
}

void BlinkView::LoadVisuallyCommitted() {
  LOG(INFO) << __func__ << "(): Load visually committed is delivered";
}

void BlinkView::TitleChanged(const std::string& title) {
  title_ = title;
  std::string progress_title = title;
  if (progress_ != "100")
    progress_title += " " + progress_ + "%";

  if (observer_)
    observer_->OnTitleChanged(this, progress_title);
}

void BlinkView::NavigationHistoryChanged() {
  LOG(INFO) << __func__
            << "(): Navigation history changed notification is delivered";
}

void BlinkView::Close() {}

bool BlinkView::DecidePolicyForResponse(bool is_main_frame,
                                        int status_code,
                                        const std::string& url,
                                        const std::string& status_text) {
  return policy_;
}

void BlinkView::SetDecidePolicyForResponse() {
  policy_ = true;
}

void BlinkView::OnBrowserControlCommand(
    const std::string& command,
    const std::vector<std::string>& arguments) {
  if (observer_)
    observer_->OnBrowserControlCommand(command, arguments);
}

void BlinkView::OnBrowserControlFunction(
    const std::string& command,
    const std::vector<std::string>& arguments,
    std::string* result) {
  if (observer_)
    *result = observer_->OnBrowserControlFunction(command, arguments);
}

void BlinkView::SetMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Set media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = true;
}

void BlinkView::ClearMediaCapturePermission() {
  LOG(INFO) << __func__ << "(): Clear media capture permission is delivered";
  accepts_video_capture_ = accepts_audio_capture_ = false;
}

bool BlinkView::AcceptsVideoCapture() {
  LOG(INFO) << __func__ << "(): Accepts video capture is delivered";
  return accepts_video_capture_;
}

bool BlinkView::AcceptsAudioCapture() {
  LOG(INFO) << __func__ << "(): Accepts audio capture is delivered";
  return accepts_audio_capture_;
}

void BlinkView::LoadStarted() {
  LOG(INFO) << __func__ << "(): Load started notification is delivered";
}

void BlinkView::LoadFinished(const std::string& url) {
  LOG(INFO) << __func__ << "(): Load finished notification is delivered";
}

void BlinkView::LoadFailed(const std::string& url,
                           int error_code,
                           const std::string& error_description) {
  LOG(INFO) << __func__ << "(): Load failed notification is delivered";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void BlinkView::LoadAborted(const std::string& url) {
  LOG(INFO) << __func__
            << "(): Load aborted notification is delivered"
            << " for url [" << url << "]";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void BlinkView::LoadStopped(const std::string& url) {
  LOG(INFO) << __func__ << "(): Load stopped notification is delivered";
  if (observer_)
    observer_->OnLoadFailed(this);
}

void BlinkView::RenderProcessCreated(int pid) {
  if (observer_)
    observer_->OnRenderProcessCreated(this);
}

void BlinkView::RenderProcessGone() {
  if (observer_)
    observer_->OnRenderProcessGone(this);
}

void BlinkView::DocumentLoadFinished() {
  LOG(INFO) << __func__
            << "(): Document load finished notification is delivered";
  if (observer_)
    observer_->OnDocumentLoadFinished(this);
}

void BlinkView::DidHistoryBackOnTopPage() {}

void BlinkView::DidClearWindowObject() {
  LOG(INFO) << __func__
            << "(): Did clear window object notification is delivered";
}

void BlinkView::DidSwapCompositorFrame() {
  LOG(INFO) << __func__
            << "(): Did swap window frame notification is delivered";
}

void WebAppWindowImpl::OnWindowClosing() {
  observer_->OnWindowClosing(this);
}

}  // namespace wam_demo
