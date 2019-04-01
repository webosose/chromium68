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

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/strings/string_number_conversions.h"
#include "components/error_page/common/localized_error.h"
#include "components/error_page/common/error.h"
#include "content/public/common/content_switches.h"
#include "content/public/renderer/render_thread.h"
#include "neva/app_runtime/renderer/app_runtime_content_renderer_client.h"

#include "neva/app_runtime/renderer/app_runtime_page_load_timing_render_frame_observer.h"
#include "neva/app_runtime/renderer/app_runtime_render_frame_observer.h"
#include "neva/app_runtime/renderer/net/app_runtime_net_error_helper.h"

#include "third_party/blink/public/platform/modules/fetch/fetch_api_request.mojom-shared.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "ui/base/resource/resource_bundle.h"

using blink::mojom::FetchCacheMode;
using blink::WebURL;
using blink::WebURLError;
using blink::WebURLRequest;
using blink::WebURLResponse;

namespace app_runtime {

void AppRuntimeContentRendererClient::RenderThreadStarted() {

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableWatchdog)) {
    watchdog_.reset(new watchdog::Watchdog());

    std::string env_timeout =
        command_line.GetSwitchValueASCII(switches::kWatchdogRendererTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      base::StringToInt(env_timeout, &timeout);
      watchdog_->SetTimeout(timeout);
    }

    std::string env_period =
        command_line.GetSwitchValueASCII(switches::kWatchdogRendererPeriod);
    if (!env_period.empty()) {
      int period;
      base::StringToInt(env_period, &period);
      watchdog_->SetPeriod(period);
    }

    watchdog_->StartWatchdog();

    // Check it's currently running on RenderThread
    CHECK(content::RenderThread::Get());
    scoped_refptr<base::SingleThreadTaskRunner> task_runner =
        base::ThreadTaskRunnerHandle::Get();
    task_runner->PostTask(
        FROM_HERE, base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                              base::Unretained(this)));
  }
}

void AppRuntimeContentRendererClient::ArmWatchdog() {
  watchdog_->Arm();
  if (!watchdog_->HasThreadInfo())
    watchdog_->SetCurrentThreadInfo();

  // Check it's currently running on RenderThread
  CHECK(content::RenderThread::Get());
  scoped_refptr<base::SingleThreadTaskRunner> task_runner =
      base::ThreadTaskRunnerHandle::Get();
  task_runner->PostDelayedTask(
      FROM_HERE,
      base::Bind(&AppRuntimeContentRendererClient::ArmWatchdog,
                 base::Unretained(this)),
      base::TimeDelta::FromSeconds(watchdog_->GetPeriod()));
}

void AppRuntimeContentRendererClient::RenderFrameCreated(
    content::RenderFrame* render_frame) {
  // Only attach AppRuntimePageLoadTimingRenderFrameObserver to the main frame,
  // since we only want to observe page load timing for the main frame.
  if (render_frame->IsMainFrame()) {
    new AppRuntimeRenderFrameObserver(render_frame);
    new AppRuntimePageLoadTimingRenderFrameObserver(render_frame);
  }
  // create net error helper
  new AppRuntimeNetErrorHelper(render_frame);
}

bool AppRuntimeContentRendererClient::ShouldSuppressErrorPage(
    content::RenderFrame* render_frame,
    const GURL& url) {
  if (render_frame &&
      AppRuntimeNetErrorHelper::Get(render_frame) &&
      AppRuntimeNetErrorHelper::Get(render_frame)
          ->ShouldSuppressErrorPage(url)) {
    return true;
  }
  return false;
}

void AppRuntimeContentRendererClient::PrepareErrorPage(
    content::RenderFrame* render_frame,
    const WebURLRequest& failed_request,
    const WebURLError& error,
    std::string* error_html,
    base::string16* error_description) {
  PrepareErrorPageInternal(
      render_frame, failed_request,
      error_page::Error::NetError(error.url(), error.reason(),
                                  error.has_copy_in_cache()),
      error_html, error_description);
}

void AppRuntimeContentRendererClient::PrepareErrorPageForHttpStatusError(
    content::RenderFrame* render_frame,
    const WebURLRequest& failed_request,
    const GURL& unreachable_url,
    int http_status,
    std::string* error_html,
    base::string16* error_description) {
  PrepareErrorPageInternal(
      render_frame, failed_request,
      error_page::Error::HttpError(unreachable_url, http_status), error_html,
      error_description);
}

void AppRuntimeContentRendererClient::GetErrorDescription(
    const blink::WebURLRequest& failed_request,
    const blink::WebURLError& error,
    base::string16* error_description) {
  GetErrorDescriptionInternal(
      failed_request, error_page::Error::NetError(error.url(), error.reason(),
                                                  error.has_copy_in_cache()),
      error_description);
}

void AppRuntimeContentRendererClient::PrepareErrorPageInternal(
    content::RenderFrame* render_frame,
    const WebURLRequest& failed_request,
    const error_page::Error& error,
    std::string* error_html,
    base::string16* error_description) {
  bool is_post = failed_request.HttpMethod().Ascii() == "POST";
  bool is_ignoring_cache =
      failed_request.GetCacheMode() == FetchCacheMode::kBypassCache;
  AppRuntimeNetErrorHelper::Get(render_frame)
      ->PrepareErrorPage(error, is_post, is_ignoring_cache, error_html);
  if (error_description)
    GetErrorDescriptionInternal(failed_request, error, error_description);
}

void AppRuntimeContentRendererClient::GetErrorDescriptionInternal(
    const blink::WebURLRequest& failed_request,
    const error_page::Error& error,
    base::string16* error_description) {
  bool is_post = failed_request.HttpMethod().Ascii() == "POST";
  if (error_description) {
    *error_description = error_page::LocalizedError::GetErrorDetails(
        error.domain(), error.reason(), is_post);
  }
}

bool AppRuntimeContentRendererClient::HasErrorPage(int http_status_code) {
  // Use an internal error page, if we have one for the status code.
  return error_page::LocalizedError::HasStrings(
      error_page::Error::kHttpErrorDomain, http_status_code);
}

void AppRuntimeContentRendererClient::OnLocaleChanged(
    const std::string& new_locale) {
  base::i18n::SetICUDefaultLocale(new_locale);
  if (ui::ResourceBundle::HasSharedInstance()) {
    ui::ResourceBundle::GetSharedInstance().ReloadLocaleResources(new_locale);
    ui::ResourceBundle::GetSharedInstance().ReloadFonts();
  }
}

}  // namespace app_runtime
