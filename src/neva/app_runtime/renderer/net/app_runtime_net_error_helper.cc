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

#include "neva/app_runtime/renderer/net/app_runtime_net_error_helper.h"

#include <memory>
#include <string>
#include <utility>

#include "base/command_line.h"
#include "base/i18n/rtl.h"
#include "base/json/json_writer.h"
#include "base/metrics/histogram.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "build/build_config.h"
#include "components/error_page/common/error_page_params.h"
#include "components/error_page/common/localized_error.h"
#include "components/error_page/common/net_error_info.h"
#include "components/url_formatter/url_formatter.h"
#include "content/public/common/content_client.h"
#include "content/public/common/url_constants.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/render_view.h"
#include "content/public/renderer/resource_fetcher.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "neva/app_runtime/common/app_runtime.mojom.h"
#include "neva/app_runtime/common/app_runtime_localized_error.h"
#include "neva/app_runtime/grit/network_error_pages.h"
#include "neva/app_runtime/renderer/net/template_builder.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/blink/public/platform/web_url.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_document.h"
#include "third_party/blink/public/web/web_frame.h"
#include "third_party/blink/public/web/web_local_frame.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/webui/jstemplate_builder.h"
#include "url/gurl.h"

using base::JSONWriter;
using content::DocumentState;
using content::RenderFrame;
using content::RenderFrameObserver;
using content::RenderThread;
using content::kUnreachableWebDataURL;
using error_page::ErrorPageParams;
using error_page::AppRuntimeLocalizedError;

namespace {

// Number of seconds to wait for the navigation correction service to return
// suggestions.  If it takes too long, just use the local error page.
const int kNavigationCorrectionFetchTimeoutSec = 3;

NetErrorHelperCore::PageType GetLoadingPageType(
    blink::WebDocumentLoader* document_loader) {
  GURL url = document_loader->GetRequest().Url();
  if (!url.is_valid() || url.spec() != kUnreachableWebDataURL)
    return NetErrorHelperCore::NON_ERROR_PAGE;
  return NetErrorHelperCore::ERROR_PAGE;
}

NetErrorHelperCore::FrameType GetFrameType(RenderFrame* render_frame) {
  if (render_frame->IsMainFrame())
    return NetErrorHelperCore::MAIN_FRAME;
  return NetErrorHelperCore::SUB_FRAME;
}

const net::NetworkTrafficAnnotationTag& GetNetworkTrafficAnnotationTag() {
  static const net::NetworkTrafficAnnotationTag network_traffic_annotation_tag =
      net::DefineNetworkTrafficAnnotation("app_runtime_net_error_helper",
                                          R"()");
  return network_traffic_annotation_tag;
}

}  // namespace

namespace app_runtime {

AppRuntimeNetErrorHelper::AppRuntimeNetErrorHelper(RenderFrame* render_frame)
    : RenderFrameObserver(render_frame),
      content::RenderFrameObserverTracker<AppRuntimeNetErrorHelper>(
          render_frame),
      weak_controller_delegate_factory_(this) {
  RenderThread::Get()->AddObserver(this);

  core_.reset(
      new NetErrorHelperCore(this, true, true, !render_frame->IsHidden()));
}

AppRuntimeNetErrorHelper::~AppRuntimeNetErrorHelper() {
  RenderThread::Get()->RemoveObserver(this);
}

void AppRuntimeNetErrorHelper::ButtonPressed(
    AppRuntimeNetErrorPageController::Button button,
    int target_id) {
  switch (button) {
    case AppRuntimeNetErrorPageController::SETTINGS_BUTTON:
      app_runtime::mojom::AppRuntimeHostAssociatedPtr interface;
      render_frame()->GetRemoteAssociatedInterfaces()->GetInterface(&interface);
      if (interface)
        interface->DoLaunchSettingsApplication(target_id);
      break;
  }
}

void AppRuntimeNetErrorHelper::DidStartProvisionalLoad(
    blink::WebDocumentLoader* loader) {
  core_->OnStartLoad(GetFrameType(render_frame()), GetLoadingPageType(loader));
}

void AppRuntimeNetErrorHelper::DidCommitProvisionalLoad(
    bool is_new_navigation,
    bool is_same_page_navigation) {
  // If this is a "same page" navigation, it's not a real navigation.  There
  // wasn't a start event for it, either, so just ignore it.
  if (is_same_page_navigation)
    return;

  // Invalidate weak pointers from old error page controllers. If loading a new
  // error page, the controller has not yet been attached, so this won't affect
  // it.
  weak_controller_delegate_factory_.InvalidateWeakPtrs();

  core_->OnCommitLoad(GetFrameType(render_frame()),
                      render_frame()->GetWebFrame()->GetDocument().Url());
}

void AppRuntimeNetErrorHelper::DidFinishLoad() {
  core_->OnFinishLoad(GetFrameType(render_frame()));
}

void AppRuntimeNetErrorHelper::OnStop() {
  core_->OnStop();
}

void AppRuntimeNetErrorHelper::WasShown() {
  core_->OnWasShown();
}

void AppRuntimeNetErrorHelper::WasHidden() {
  core_->OnWasHidden();
}

void AppRuntimeNetErrorHelper::OnDestruct() {
  delete this;
}

void AppRuntimeNetErrorHelper::NetworkStateChanged(bool enabled) {
  core_->NetworkStateChanged(enabled);
}

void AppRuntimeNetErrorHelper::PrepareErrorPage(const error_page::Error& error,
                                                bool is_failed_post,
                                                bool is_ignoring_cache,
                                                std::string* error_html) {
  core_->PrepareErrorPage(GetFrameType(render_frame()), error, is_failed_post,
                          is_ignoring_cache, error_html);
}

bool AppRuntimeNetErrorHelper::ShouldSuppressErrorPage(const GURL& url) {
  return core_->ShouldSuppressErrorPage(GetFrameType(render_frame()), url);
}

void AppRuntimeNetErrorHelper::GenerateLocalizedErrorPage(
    const error_page::Error& error,
    bool is_failed_post,
    bool can_show_network_diagnostics_dialog,
    std::unique_ptr<error_page::ErrorPageParams> params,
    bool* reload_button_shown,
    bool* show_saved_copy_button_shown,
    bool* show_cached_copy_button_shown,
    bool* download_button_shown,
    std::string* html) const {
  html->clear();

  // Resource will change to net error specific page
  int resource_id = IDR_APP_RUNTIME_NETWORK_ERROR_PAGE;
  const base::StringPiece template_html(
      ui::ResourceBundle::GetSharedInstance().GetRawDataResource(resource_id));
  if (template_html.empty()) {
    NOTREACHED() << "unable to load template.";
  } else {
    base::DictionaryValue error_strings;
    AppRuntimeLocalizedError::GetStrings(
        error.reason(), error.domain(), error.url(), is_failed_post,
        (error.stale_copy_in_cache() && !is_failed_post),
        RenderThread::Get()->GetLocale(),
        render_frame()->GetRenderView()->GetAcceptLanguages(),
        std::move(params), &error_strings);
    *reload_button_shown = error_strings.Get("reloadButton", nullptr);
    *show_saved_copy_button_shown =
        error_strings.Get("showSavedCopyButton", nullptr);
    *show_cached_copy_button_shown = error_strings.Get("cacheButton", nullptr);
    *download_button_shown = error_strings.Get("downloadButton", nullptr);

    // "t" is the id of the template's root node.
    *html = webui::GetTemplatesHtml(template_html, &error_strings, "t");

    // viewport width and height
    int viewport_width = render_frame()->GetWebFrame()->viewportSize().width;
    int viewport_height = render_frame()->GetWebFrame()->viewportSize().height;

    // Add webos specific functionality
    *html = app_runtime::GetTemplatesHtml(*html, &error_strings, error.reason(),
                                          "t", viewport_width, viewport_height);
  }
}

void AppRuntimeNetErrorHelper::LoadErrorPage(const std::string& html,
                                             const GURL& failed_url) {
  render_frame()->GetWebFrame()->LoadHTMLString(
      html, GURL(kUnreachableWebDataURL), failed_url, true);
}

void AppRuntimeNetErrorHelper::EnablePageHelperFunctions(net::Error net_error) {
  AppRuntimeNetErrorPageController::Install(
      render_frame(), weak_controller_delegate_factory_.GetWeakPtr());
}

void AppRuntimeNetErrorHelper::UpdateErrorPage(
    const error_page::Error& error,
    bool is_failed_post,
    bool can_show_network_diagnostics_dialog) {
  base::DictionaryValue error_strings;
  error_page::AppRuntimeLocalizedError::GetStrings(
      error.reason(), error.domain(), error.url(), is_failed_post,
      (error.stale_copy_in_cache() && !is_failed_post),
      RenderThread::Get()->GetLocale(),
      render_frame()->GetRenderView()->GetAcceptLanguages(),
      std::unique_ptr<ErrorPageParams>(), &error_strings);

  std::string json;
  JSONWriter::Write(error_strings, &json);

  std::string js =
      "if (window.updateForDnsProbe) "
      "updateForDnsProbe(" +
      json + ");";
  base::string16 js16;
  if (!base::UTF8ToUTF16(js.c_str(), js.length(), &js16)) {
    NOTREACHED();
    return;
  }

  render_frame()->ExecuteJavaScript(js16);
}

void AppRuntimeNetErrorHelper::FetchNavigationCorrections(
    const GURL& navigation_correction_url,
    const std::string& navigation_correction_request_body) {
  DCHECK(!correction_fetcher_.get());

  correction_fetcher_ =
      content::ResourceFetcher::Create(navigation_correction_url);
  correction_fetcher_->SetMethod("POST");
  correction_fetcher_->SetBody(navigation_correction_request_body);
  correction_fetcher_->SetHeader("Content-Type", "application/json");

  // Prevent CORB from triggering on this request by setting an Origin header.
  correction_fetcher_->SetHeader("Origin", "null");

  correction_fetcher_->Start(
      render_frame()->GetWebFrame(),
      blink::WebURLRequest::kRequestContextInternal,
      render_frame()->GetURLLoaderFactory(), GetNetworkTrafficAnnotationTag(),
      base::BindOnce(&AppRuntimeNetErrorHelper::OnNavigationCorrectionsFetched,
                     base::Unretained(this)));

  correction_fetcher_->SetTimeout(
      base::TimeDelta::FromSeconds(kNavigationCorrectionFetchTimeoutSec));
}

void AppRuntimeNetErrorHelper::CancelFetchNavigationCorrections() {
  correction_fetcher_.reset();
}

void AppRuntimeNetErrorHelper::SendTrackingRequest(
    const GURL& tracking_url,
    const std::string& tracking_request_body) {
  // If there's already a pending tracking request, this will cancel it.
  tracking_fetcher_ = content::ResourceFetcher::Create(tracking_url);
  tracking_fetcher_->SetMethod("POST");
  tracking_fetcher_->SetBody(tracking_request_body);
  tracking_fetcher_->SetHeader("Content-Type", "application/json");

  tracking_fetcher_->Start(
      render_frame()->GetWebFrame(),
      blink::WebURLRequest::kRequestContextInternal,
      render_frame()->GetURLLoaderFactory(), GetNetworkTrafficAnnotationTag(),
      base::BindOnce(&AppRuntimeNetErrorHelper::OnTrackingRequestComplete,
                     base::Unretained(this)));
}

void AppRuntimeNetErrorHelper::ReloadPage(bool bypass_cache) {
  render_frame()->GetWebFrame()->Reload(
      bypass_cache ? blink::WebFrameLoadType::kReloadBypassingCache
                   : blink::WebFrameLoadType::kReload);
}

void AppRuntimeNetErrorHelper::OnNavigationCorrectionsFetched(
    const blink::WebURLResponse& response,
    const std::string& data) {
  // The fetcher may only be deleted after |data| is passed to |core_|.  Move
  // it to a temporary to prevent any potential re-entrancy issues.
  std::unique_ptr<content::ResourceFetcher> fetcher(
      correction_fetcher_.release());
  bool success = (!response.IsNull() && response.HttpStatusCode() == 200);
  core_->OnNavigationCorrectionsFetched(success ? data : "",
                                        base::i18n::IsRTL());
}

void AppRuntimeNetErrorHelper::LoadPageFromCache(const GURL& page_url) {
  blink::WebLocalFrame* web_frame = render_frame()->GetWebFrame();
  DCHECK_NE("POST",
            web_frame->GetDocumentLoader()->GetRequest().HttpMethod().Ascii());

  blink::WebURLRequest request(page_url);
  request.SetCacheMode(blink::mojom::FetchCacheMode::kOnlyIfCached);
  request.SetRequestorOrigin(blink::WebSecurityOrigin::Create(page_url));
  web_frame->LoadRequest(request);
}

void AppRuntimeNetErrorHelper::OnTrackingRequestComplete(
    const blink::WebURLResponse& response,
    const std::string& data) {
  tracking_fetcher_.reset();
}

}  // namespace app_runtime
