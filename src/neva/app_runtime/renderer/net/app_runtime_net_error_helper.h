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

#ifndef NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_HELPER_H_
#define NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_HELPER_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "build/build_config.h"
#include "chrome/renderer/net/net_error_helper_core.h"
#include "components/error_page/common/net_error_info.h"
#include "content/public/renderer/render_frame_observer.h"
#include "content/public/renderer/render_frame_observer_tracker.h"
#include "content/public/renderer/render_thread_observer.h"
#include "net/base/net_errors.h"
#include "neva/app_runtime/renderer/net/app_runtime_net_error_page_controller.h"

class GURL;

namespace blink {
class WebURLResponse;
}

namespace content {
class ResourceFetcher;
}

namespace error_page {
struct ErrorPageParams;
}

namespace app_runtime {
// Listens for NetErrorInfo messages from the NetErrorTabHelper on the
// browser side and updates the error page with more details (currently, just
// DNS probe results) if/when available.
// TODO(crbug.com/578770): Should this class be moved into the error_page
// component?
class AppRuntimeNetErrorHelper
    : public content::RenderFrameObserver,
      public content::RenderFrameObserverTracker<AppRuntimeNetErrorHelper>,
      public content::RenderThreadObserver,
      public NetErrorHelperCore::Delegate,
      public AppRuntimeNetErrorPageController::Delegate {
 public:
  explicit AppRuntimeNetErrorHelper(content::RenderFrame* render_frame);
  ~AppRuntimeNetErrorHelper() override;

  // AppRuntimeNetErrorPageController::Delegate implementation
  void ButtonPressed(AppRuntimeNetErrorPageController::Button button,
                     int target_id) override;

  // RenderFrameObserver implementation.
  void DidStartProvisionalLoad(blink::WebDocumentLoader* loader) override;
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_page_navigation) override;
  void DidFinishLoad() override;
  void OnStop() override;
  void WasShown() override;
  void WasHidden() override;
  void OnDestruct() override;

  // RenderThreadObserver implementation.
  void NetworkStateChanged(bool online) override;

  // Sets values in |pending_error_page_info_|. If |error_html| is not null, it
  // initializes |error_html| with the HTML of an error page in response to
  // |error|.  Updates internals state with the assumption the page will be
  // loaded immediately.
  void PrepareErrorPage(const error_page::Error& error,
                        bool is_failed_post,
                        bool is_ignoring_cache,
                        std::string* error_html);

  // Returns whether a load for |url| in the |frame| the
  // AppRuntimeNetErrorHelper is
  // attached to should have its error page suppressed.
  bool ShouldSuppressErrorPage(const GURL& url);

 private:
  // NetErrorHelperCore::Delegate implementation:
  void GenerateLocalizedErrorPage(
      const error_page::Error& error,
      bool is_failed_post,
      bool can_show_network_diagnostics_dialog,
      std::unique_ptr<error_page::ErrorPageParams> params,
      bool* reload_button_shown,
      bool* show_saved_copy_button_shown,
      bool* show_cached_copy_button_shown,
      bool* download_button_shown,
      std::string* html) const override;
  void LoadErrorPage(const std::string& html, const GURL& failed_url) override;
  void EnablePageHelperFunctions(net::Error net_error) override;
  void UpdateErrorPage(const error_page::Error& error,
                       bool is_failed_post,
                       bool can_show_network_diagnostics_dialog) override;
  void FetchNavigationCorrections(
      const GURL& navigation_correction_url,
      const std::string& navigation_correction_request_body) override;
  void CancelFetchNavigationCorrections() override;
  void SendTrackingRequest(const GURL& tracking_url,
                           const std::string& tracking_request_body) override;
  void ReloadPage(bool bypass_cache) override;
  void LoadPageFromCache(const GURL& page_url) override;
  void DiagnoseError(const GURL& page_url) override {}
  void DownloadPageLater() override {}
  void SetIsShowingDownloadButton(bool show) override {}

  void OnNavigationCorrectionsFetched(const blink::WebURLResponse& response,
                                      const std::string& data);

  void OnTrackingRequestComplete(const blink::WebURLResponse& response,
                                 const std::string& data);

  std::unique_ptr<content::ResourceFetcher> correction_fetcher_;
  std::unique_ptr<content::ResourceFetcher> tracking_fetcher_;

  std::unique_ptr<NetErrorHelperCore> core_;

  // Weak factory for vending a weak pointer to a NetErrorPageController. Weak
  // pointers are invalidated on each commit, to prevent getting messages from
  // Controllers used for the previous commit that haven't yet been cleaned up.
  base::WeakPtrFactory<AppRuntimeNetErrorPageController::Delegate>
      weak_controller_delegate_factory_;
  DISALLOW_COPY_AND_ASSIGN(AppRuntimeNetErrorHelper);
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_RENDERER_NET_APP_RUNTIME_NET_ERROR_HELPER_H_
