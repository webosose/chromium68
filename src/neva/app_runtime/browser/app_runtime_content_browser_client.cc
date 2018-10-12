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

#include "neva/app_runtime/browser/app_runtime_content_browser_client.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/neva/base_switches.h"
#include "base/strings/utf_string_conversions.h"
#include "cc/base/switches_neva.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/devtools_manager_delegate.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_process_host.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/app_runtime_web_contents_view_delegate_creator.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "neva/app_runtime/webview.h"
#include "services/service_manager/sandbox/switches.h"

namespace app_runtime {

AppRuntimeContentBrowserClient::AppRuntimeContentBrowserClient(
    net::NetworkDelegate* delegate)
    : url_request_context_factory_(new URLRequestContextFactory(delegate)),
#if defined(ENABLE_PLUGINS)
      plugin_loaded_(false),
#endif
      external_browser_context_(nullptr) {
}

AppRuntimeContentBrowserClient::AppRuntimeContentBrowserClient(
    content::BrowserContext* p,
    net::NetworkDelegate* delegate)
    : AppRuntimeContentBrowserClient(delegate) {
  external_browser_context_ = p;
}

AppRuntimeContentBrowserClient::~AppRuntimeContentBrowserClient() {}

void AppRuntimeContentBrowserClient::SetBrowserExtraParts(
    AppRuntimeBrowserMainExtraParts* browser_extra_parts) {
  browser_extra_parts_ = browser_extra_parts;
}

content::BrowserMainParts*
AppRuntimeContentBrowserClient::CreateBrowserMainParts(
    const content::MainFunctionParams& parameters) {
  main_parts_ = new AppRuntimeBrowserMainParts(
      url_request_context_factory_.get(), external_browser_context_);

  if (browser_extra_parts_)
    main_parts_->AddParts(browser_extra_parts_);

  return main_parts_;
}

content::WebContentsViewDelegate*
AppRuntimeContentBrowserClient::GetWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return CreateAppRuntimeWebContentsViewDelegate(web_contents);
}

void AppRuntimeContentBrowserClient::AllowCertificateError(
    content::WebContents* web_contents,
    int cert_error,
    const net::SSLInfo& ssl_info,
    const GURL& request_url,
    content::ResourceType resource_type,
    bool strict_enforcement,
    bool expired_previous_decision,
    const base::Callback<void(content::CertificateRequestResultType)>&
        callback) {
  // HCAP requirements: For SSL Certificate error, follows the policy settings
  if (web_contents && web_contents->GetDelegate()) {
    WebView* webView = static_cast<WebView*>(web_contents->GetDelegate());
    switch (webView->GetSSLCertErrorPolicy()) {
      case SSL_CERT_ERROR_POLICY_IGNORE:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_CONTINUE);
        return;
      case SSL_CERT_ERROR_POLICY_DENY:
        callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
        return;
      default:
        break;
    }
  }

  if (resource_type != content::RESOURCE_TYPE_MAIN_FRAME) {
    // A sub-resource has a certificate error. The user doesn't really
    // have a context for making the right decision, so block the
    // request hard, without and info bar to allow showing the insecure
    // content.
    callback.Run(content::CERTIFICATE_REQUEST_RESULT_TYPE_DENY);
  }
}

void AppRuntimeContentBrowserClient::AppendExtraCommandLineSwitches(
    base::CommandLine* command_line,
    int child_process_id) {
  command_line->AppendSwitch(service_manager::switches::kNoSandbox);

  // Append v8 snapshot path if exists
  auto iter = v8_snapshot_pathes_.find(child_process_id);
  if (iter != v8_snapshot_pathes_.end()) {
    command_line->AppendSwitchPath(switches::kV8SnapshotBlobPath,
                                   base::FilePath(iter->second));
    v8_snapshot_pathes_.erase(iter);
  }

  // Append v8 extra flags if exists
  iter = v8_extra_flags_.find(child_process_id);
  if (iter != v8_extra_flags_.end()) {
    std::string js_flags = iter->second;
    // If already has, append it also
    if (command_line->HasSwitch(switches::kJavaScriptFlags)) {
      js_flags.append(" ");
      js_flags.append(
          command_line->GetSwitchValueASCII(switches::kJavaScriptFlags));
    }
    command_line->AppendSwitchASCII(switches::kJavaScriptFlags, js_flags);
    v8_extra_flags_.erase(iter);
  }

  // Append native scroll related flags if native scroll is on by appinfo.json
  auto iter_ns = use_native_scroll_map_.find(child_process_id);
  if (iter_ns != use_native_scroll_map_.end()) {
    bool use_native_scroll = iter_ns->second;
    if (use_native_scroll) {
      // Enables EnableNativeScroll, which is only enabled when there is
      // 'useNativeScroll': true in appinfo.json. If this flag is enabled,
      // Duration of the scroll offset animation is modified.
      if (!command_line->HasSwitch(cc::switches::kEnableWebOSNativeScroll))
        command_line->AppendSwitch(cc::switches::kEnableWebOSNativeScroll);

      // Enables SmoothScrolling, which is mandatory to enable
      // CSSOMSmoothScroll.
      if (!command_line->HasSwitch(switches::kEnableSmoothScrolling))
        command_line->AppendSwitch(switches::kEnableSmoothScrolling);

      // Adds CSSOMSmoothScroll to EnableBlinkFeatures.
      std::string enable_blink_features_flags = "CSSOMSmoothScroll";
      if (command_line->HasSwitch(switches::kEnableBlinkFeatures)) {
        enable_blink_features_flags.append(",");
        enable_blink_features_flags.append(
            command_line->GetSwitchValueASCII(switches::kEnableBlinkFeatures));
      }
      command_line->AppendSwitchASCII(switches::kEnableBlinkFeatures,
                                      enable_blink_features_flags);

      // Enables PreferCompositingToLCDText. If this flag is enabled, Compositor
      // thread handles scrolling and disable LCD-text(AntiAliasing) in the
      // scroll area.
      // See PaintLayerScrollableArea.cpp::layerNeedsCompositingScrolling()
      if (!command_line->HasSwitch(switches::kEnablePreferCompositingToLCDText))
        command_line->AppendSwitch(switches::kEnablePreferCompositingToLCDText);
    }

    use_native_scroll_map_.erase(iter_ns);
  }
}

void AppRuntimeContentBrowserClient::SetUseNativeScroll(
    int child_process_id,
    bool use_native_scroll) {
  use_native_scroll_map_.insert(
      std::pair<int, bool>(child_process_id, use_native_scroll));
}

content::DevToolsManagerDelegate*
AppRuntimeContentBrowserClient::GetDevToolsManagerDelegate() {
  return new AppRuntimeDevToolsManagerDelegate();
}

void AppRuntimeContentBrowserClient::OverrideWebkitPrefs(
    content::RenderViewHost* render_view_host,
    content::WebPreferences* prefs) {
  if (!render_view_host)
    return;

  RenderViewHostDelegate* delegate = render_view_host->GetDelegate();
  if (delegate)
    delegate->OverrideWebkitPrefs(prefs);
}

void AppRuntimeContentBrowserClient::SetV8SnapshotPath(
    int child_process_id,
    const std::string& path) {
  v8_snapshot_pathes_.insert(
      std::make_pair(child_process_id, path));
}

void AppRuntimeContentBrowserClient::SetV8ExtraFlags(int child_process_id,
                                                     const std::string& flags) {
  v8_extra_flags_.insert(std::make_pair(child_process_id, flags));
}

void AppRuntimeContentBrowserClient::SetApplicationLocale(
    const std::string& locale) {
  if (current_locale_ == locale)
    return;

  current_locale_ = locale;
  for (content::RenderProcessHost::iterator it(
           content::RenderProcessHost::AllHostsIterator());
       !it.IsAtEnd(); it.Advance()) {
    it.GetCurrentValue()->OnLocaleChanged(locale);
  }
}

}  // namespace app_runtime
