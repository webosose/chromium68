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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_

#include "content/public/browser/content_browser_client.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"

namespace net {
class NetworkDelegate;
}  // namespace net

namespace app_runtime {

class AppRuntimeBrowserMainExtraParts;
class URLRequestContextFactory;

class AppRuntimeContentBrowserClient : public content::ContentBrowserClient {
 public:
  explicit AppRuntimeContentBrowserClient(net::NetworkDelegate* delegate);
  AppRuntimeContentBrowserClient(content::BrowserContext* p,
                                 net::NetworkDelegate* delegate);
  ~AppRuntimeContentBrowserClient() override;

  void SetBrowserExtraParts(
    AppRuntimeBrowserMainExtraParts* browser_extra_parts);

  // content::ContentBrowserClient implementations.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;

  void AllowCertificateError(
      content::WebContents* web_contents,
      int cert_error,
      const net::SSLInfo& ssl_info,
      const GURL& request_url,
      content::ResourceType resource_type,
      bool strict_enforcement,
      bool expired_previous_decision,
      const base::Callback<void(content::CertificateRequestResultType)>&
          callback) override;

  content::WebContentsViewDelegate* GetWebContentsViewDelegate(
      content::WebContents* web_contents) override;

  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;

  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  std::string GetApplicationLocale() override { return current_locale_; }

  void OverrideWebkitPrefs(content::RenderViewHost* render_view_host,
                           content::WebPreferences* prefs) override;

  AppRuntimeBrowserMainParts* GetMainParts() { return main_parts_; }

  void SetDoNotTrack(bool dnt) { do_not_track_ = dnt; }
  bool DoNotTrack() { return do_not_track_; }

#if defined(ENABLE_PLUGINS)
  bool PluginLoaded() const { return plugin_loaded_; }
  void SetPluginLoaded(bool loaded) { plugin_loaded_ = loaded; }
#endif

  void SetV8SnapshotPath(int child_process_id, const std::string& path);
  void SetV8ExtraFlags(int child_process_id, const std::string& flags);
  void SetUseNativeScroll(int child_process_id, bool use_native_scroll);
  void SetApplicationLocale(const std::string& locale);

 private:
  class MainURLRequestContextGetter;

  AppRuntimeBrowserMainExtraParts* browser_extra_parts_ = nullptr;
  std::unique_ptr<URLRequestContextFactory> url_request_context_factory_;
  AppRuntimeBrowserMainParts* main_parts_;

  content::BrowserContext* external_browser_context_;
  bool do_not_track_;

#if defined(ENABLE_PLUGINS)
  bool plugin_loaded_;
#endif

  std::map<int, std::string> v8_snapshot_pathes_;
  std::map<int, std::string> v8_extra_flags_;

  // Stores (int child_process_id, bool use_native_scroll) and apply the flags
  // related to native scroll when use_native_scroll flag for the render process
  // is true.
  std::map<int, bool> use_native_scroll_map_;
  std::string current_locale_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_CONTENT_BROWSER_CLIENT_H_
