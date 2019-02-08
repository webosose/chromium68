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

#ifndef NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_BASE_H_
#define NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_BASE_H_

#include <string>
#include <memory>

#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/public/app_runtime_export.h"
#include "neva/app_runtime/public/webview_base_internals.h"
#include "neva/app_runtime/public/webview_delegate.h"

namespace app_runtime {

class AppRuntimeEvent;
class WebView;
class WebViewBaseObserver;
class WebViewProfile;

class APP_RUNTIME_EXPORT WebViewBase : public WebViewDelegate,
                                       public internals::WebViewBaseInternals {
 public:
  enum FontRenderParams {
    HINTING_NONE,
    HINTING_SLIGHT,
    HINTING_MEDIUM,
    HINTING_FULL
  };

  enum MemoryPressureLevel {
    MEMORY_PRESSURE_NONE = 0,
    MEMORY_PRESSURE_LOW = 1,
    MEMORY_PRESSURE_CRITICAL = 2
  };

  static void SetFileAccessBlocked(bool blocked);

  WebViewBase(int width = 1920, int height = 1080, WebViewProfile* profile = nullptr);
  virtual ~WebViewBase();

  // WebViewBaseInternals
  content::WebContents* GetWebContents() override;

  void AddUserStyleSheet(const std::string& sheet);
  std::string DefaultUserAgent() const;
  std::string UserAgent() const;
  void LoadUrl(const std::string& url);
  void StopLoading();
  void LoadExtension(const std::string& name);
  void ReplaceBaseURL(const std::string& new_url, const std::string& old_url);
  void EnableInspectablePage();
  int DevToolsPort() const;
  void SetInspectable(bool enable);
  void AddAvailablePluginDir(const std::string& directory);
  void AddCustomPluginDir(const std::string& directory);
  void SetUserAgent(const std::string& useragent);
  void SetBackgroundColor(int r, int g, int b, int alpha);
  void SetShouldSuppressDialogs(bool suppress);
  void SetUseAccessibility(bool enabled);
  void SetActiveOnNonBlankPaint(bool active);
  void SetViewportSize(int width, int height);
  void NotifyMemoryPressure(MemoryPressureLevel level);
  void SetVisible(bool visible);
  void SetVisibilityState(WebPageVisibilityState visibility_state);
  void DeleteWebStorages(const std::string& identifier);
  std::string DocumentTitle() const;
  void SuspendWebPageDOM();
  void ResumeWebPageDOM();
  void SuspendWebPageMedia();
  void ResumeWebPageMedia();
  void SuspendPaintingAndSetVisibilityHidden();
  void ResumePaintingAndSetVisibilityVisible();
  void CommitLoadVisually();
  void RunJavaScript(const std::string& js_code);
  void RunJavaScriptInAllFrames(const std::string& js_code);
  void Reload();
  int RenderProcessPid() const;
  bool IsDrmEncrypted(const std::string& url);
  std::string DecryptDrm(const std::string& url);
  void SetFocus(bool focus);
  double GetZoomFactor();
  void SetZoomFactor(double factor);
  void SetDoNotTrack(bool dnt);
  void ForwardAppRuntimeEvent(AppRuntimeEvent* event);
  bool CanGoBack() const;
  void GoBack();
  void RequestGetCookies(const std::string& url);
  void SetAdditionalContentsScale(float scale_x, float scale_y);
  void SetHardwareResolution(int width, int height);
  void SetEnableHtmlSystemKeyboardAttr(bool enabled);
  void RequestInjectionLoading(const std::string& injection_name);
  void RequestClearInjections();
  void DropAllPeerConnections(DropPeerConnectionReason reason);

  const std::string& GetUrl();

  // RenderViewHost
  void SetUseLaunchOptimization(bool enabled, int delay_ms);
  void SetUseEnyoOptimization(bool enabled);
  void SetAppPreloadHint(bool is_preload);
  void SetTransparentBackground(bool enabled);

  // RenderPreference
  void SetAllowFakeBoldText(bool allow);
  void SetAppId(const std::string& appId);
  void SetSecurityOrigin(const std::string& identifier);
  void SetAcceptLanguages(const std::string& lauguages);
  void SetBoardType(const std::string& board_type);
  void SetMediaCodecCapability(const std::string& capability);
  void SetSearchKeywordForCustomPlayer(bool enabled);
  void SetSupportDolbyHDRContents(bool support);

  // WebPreferences
  void SetAllowRunningInsecureContent(bool enable);
  void SetAllowScriptsToCloseWindows(bool enable);
  void SetAllowUniversalAccessFromFileUrls(bool enable);
  void SetRequestQuotaEnabled(bool enable);
  void SetSuppressesIncrementalRendering(bool enable);
  void SetDisallowScrollbarsInMainFrame(bool enable);
  void SetDisallowScrollingInMainFrame(bool enable);
  void SetJavascriptCanOpenWindows(bool enable);
  void SetSpatialNavigationEnabled(bool enable);
  void SetSupportsMultipleWindows(bool enable);
  void SetCSSNavigationEnabled(bool enable);
  void SetV8DateUseSystemLocaloffset(bool use);
  void SetAllowLocalResourceLoad(bool enable);
  void SetLocalStorageEnabled(bool enable);
  void SetDatabaseIdentifier(const std::string& identifier);
  void SetWebSecurityEnabled(bool enable);
  void SetKeepAliveWebApp(bool enable);
  void SetAdditionalFontFamilyEnabled(bool enable);

  // FontFamily
  void SetStandardFontFamily(const std::string& font);
  void SetFixedFontFamily(const std::string& font);
  void SetSerifFontFamily(const std::string& font);
  void SetSansSerifFontFamily(const std::string& font);
  void SetCursiveFontFamily(const std::string& font);
  void SetFantasyFontFamily(const std::string& font);
  void SetFontHinting(WebViewBase::FontRenderParams hinting);
  void LoadAdditionalFont(const std::string& url, const std::string& font);

  void SetSSLCertErrorPolicy(SSLCertErrorPolicy policy);
  SSLCertErrorPolicy GetSSLCertErrorPolicy();

  // Profile
  WebViewProfile* GetProfile() const;
  void SetProfile(WebViewProfile* profile);

 private:
  WebView* webview_;
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_PUBLIC_WEBVIEW_BASE_H_
