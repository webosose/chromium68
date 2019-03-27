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

#ifndef WEBOS_WEBVIEW_BASE_H_
#define WEBOS_WEBVIEW_BASE_H_

#include <memory>
#include <string>
#include <vector>

#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/public/app_runtime_export.h"
#include "neva/app_runtime/public/injection_browser_control_handler.h"
#include "neva/app_runtime/public/webview_base_internals.h"
#include "neva/app_runtime/public/webview_delegate.h"
#include "webos/common/webos_constants.h"
#include "webos/common/webos_export.h"

namespace content {
class WebContents;
}  // namespace content

namespace app_runtime {
class AppRuntimeEvent;
class WebView;
class WebViewProfile;
}  // namespace appruntime

class WebOSEvent;

namespace webos {

class WebViewBaseObserver;

class WEBOS_EXPORT WebViewBase
    : public app_runtime::WebViewDelegate,
      public app_runtime::internals::WebViewBaseInternals,
      public app_runtime::InjectionBrowserControlHandlerDelegate {
 public:
  enum FontRenderParams {
    HINTING_NONE,
    HINTING_SLIGHT,
    HINTING_MEDIUM,
    HINTING_FULL
  };

  // Originally, WebPageVisibilityState.h, PageVisibilityState.h
  enum WebPageVisibilityState {
    WebPageVisibilityStateVisible,
    WebPageVisibilityStateHidden,
    WebPageVisibilityStateLaunching,
    WebPageVisibilityStatePrerender,
    WebPageVisibilityStateLast = WebPageVisibilityStatePrerender
  };

  enum MemoryPressureLevel {
      MEMORY_PRESSURE_NONE = 0,
      MEMORY_PRESSURE_LOW = 1,
      MEMORY_PRESSURE_CRITICAL = 2
  };

  struct ProxySettings {
    bool enabled;
    std::string mode;
    std::string ip;
    std::string port;
    std::string username;
    std::string password;
    std::string bypass_list;
  };

  static void SetFileAccessBlocked(bool blocked);

  WebViewBase(int width = 1920, int height = 1080);
  virtual ~WebViewBase();

  void Initialize(const std::string& app_id,
                  const std::string& app_path,
                  const std::string& trust_level,
                  const std::string& v8_snapshot_path,
                  const std::string& v8_extra_flags,
                  bool use_native_scroll = false);

  void AddUserStyleSheet(const std::string& sheet);
  std::string DefaultUserAgent() const;
  std::string UserAgent() const;
  void LoadUrl(const std::string& url);
  void StopLoading();
  void LoadExtension(const std::string& name);
  void ClearExtensions();
  void ReplaceBaseURL(const std::string& new_url, const std::string& old_url);
  void EnableInspectablePage();
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
  void SetProxyServer(const ProxySettings& proxy_settings);
  void SetPrerenderState();
  void SetVisibilityState(WebPageVisibilityState visibilityState);
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
  void ForwardAppRuntimeEvent(app_runtime::AppRuntimeEvent* event);
  void ForwardWebOSEvent(WebOSEvent* event);
  bool CanGoBack() const;
  void GoBack();
  void RequestGetCookies(const std::string& url);
  bool IsInputMethodActive();
  void SetAdditionalContentsScale(float scale_x, float scale_y);
  void SetHardwareResolution(int width, int height);
  void SetEnableHtmlSystemKeyboardAttr(bool enabled);
  void RequestInjectionLoading(const std::string& injection_name);
  void DropAllPeerConnections(DropPeerConnectionReason reason);

  const std::string& GetUrl();

  // WebViewBaseInternals
  content::WebContents* GetWebContents() override;

  virtual void HandleBrowserControlCommand(
      const std::string& command,
      const std::vector<std::string>& arguments);
  virtual void HandleBrowserControlFunction(
      const std::string& command,
      const std::vector<std::string>& arguments,
      std::string* result);

  // RenderViewHost
  void SetUseLaunchOptimization(bool enabled, int delay_ms);
  void SetUseEnyoOptimization(bool enabled);
  void SetAppPreloadHint(bool is_preload);
  void SetTransparentBackground(bool enabled);
  void ResetStateToMarkNextPaintForContainer();

  // RenderPreference
  void SetAllowFakeBoldText(bool allow);
  void SetAppId(const std::string& appId);
  void SetSecurityOrigin(const std::string& identifier);
  void SetAcceptLanguages(const std::string& lauguages);
  void SetBoardType(const std::string& board_type);
  void SetMediaCodecCapability(const std::string& capability);
  void SetSearchKeywordForCustomPlayer(bool enabled);
  void SetSupportDolbyHDRContents(bool support);
  void SetUseUnlimitedMediaPolicy(bool enabled);

  // WebPreferences
  void SetAllowRunningInsecureContent(bool enable);
  void SetAllowScriptsToCloseWindows(bool enable);
  void SetAllowUniversalAccessFromFileUrls(bool enable);
  void SetRequestQuotaEnabled(bool enable);
  void SetSuppressesIncrementalRendering(bool enable);
  void SetDisallowScrollbarsInMainFrame(bool enable);
  void SetDisallowScrollingInMainFrame(bool enable);
#if defined(HBBTV)
  // Stubs for hbbtv2-pluggable
  void SetHbbtvEnabled(bool on);
  void SetHbbtvAdvancedAdInsertionEnabled(bool on);
  void SetHbbtvClearMediaPlayerEnabled(bool on);
  bool GetHbbtvClearMediaPlayerEnabled();
#endif
  void SetJavascriptCanOpenWindows(bool enable);
  void SetSpatialNavigationEnabled(bool enable);
  void SetSupportsMultipleWindows(bool enable);
  void SetCSSNavigationEnabled(bool enable);
  void SetV8DateUseSystemLocaloffset(bool use);
  void SetAllowLocalResourceLoad(bool enable);
  void SetLocalStorageEnabled(bool enable);
  void SetDatabaseIdentifier(const std::string& identifier);
  void SetWebSecurityEnabled(bool enable);
  void SetXFrameOptionsCrossOriginAllowed(bool enable);
  void SetKeepAliveWebApp(bool enable);
  void SetAdditionalFontFamilyEnabled(bool enable);
  void SetForceVideoTexture(bool enable);
  void SetNotifyFMPDirectly(bool enable);
  void SetNetworkStableTimeout(double timeout);

  // FontFamily
  void SetStandardFontFamily(const std::string& font);
  void SetFixedFontFamily(const std::string& font);
  void SetSerifFontFamily(const std::string& font);
  void SetSansSerifFontFamily(const std::string& font);
  void SetCursiveFontFamily(const std::string& font);
  void SetFantasyFontFamily(const std::string& font);
  void SetFontHinting(WebViewBase::FontRenderParams hinting);
  void LoadAdditionalFont(const std::string& url, const std::string& font);

  void UpdatePreferences();

  void SetAudioGuidanceOn(bool on);
  void SetTrustLevel(const std::string& trust_level);
  void SetAppPath(const std::string& app_path);
  void SetBackHistoryAPIDisabled(const bool on);
  void SetV8SnapshotPath(const std::string& v8_snapshot_path);
  void SetV8ExtraFlags(const std::string& v8_extra_flags);
  void SetUseNativeScroll(bool use_native_scroll);

  // WebViewDelegate
  void DidFirstNonBlankPaint() override {};
  void DidDropAllPeerConnections(app_runtime::DropPeerConnectionReason) final;
  // WebViewDelegate compatibility
  virtual void DidDropAllPeerConnections(webos::DropPeerConnectionReason) = 0;

  // WebAppInjectionTxtMsgHandlerDelegate
  void OnBrowserControlCommand(
      const std::string& command,
      const std::vector<std::string>& arguments) override;

  void OnBrowserControlFunction(
      const std::string& command,
      const std::vector<std::string>& arguments,
      std::string* result) override;


  void SetSSLCertErrorPolicy(app_runtime::SSLCertErrorPolicy policy);
  app_runtime::SSLCertErrorPolicy GetSSLCertErrorPolicy();

  // Profile
  app_runtime::WebViewProfile* GetProfile() const;
  void SetProfile(app_runtime::WebViewProfile* profile);

 private:
  static app_runtime::WebPageVisibilityState FromNativeVisibilityState(
    WebPageVisibilityState visibility_state);
  void PushStateToIOThread();
  void RemoveStateFromIOThread(content::WebContents* web_contents);

  app_runtime::WebView* webview_;
  std::unique_ptr<app_runtime::InjectionBrowserControlHandler>
      injection_browser_control_handler_;
  std::string accept_language_;
  std::string app_path_;
  std::string trust_level_;
};

}  // namespace webos

#endif  // WEBOS_WEBVIEW_BASE_H_
