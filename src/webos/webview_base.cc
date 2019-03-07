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

#include "webos/webview_base.h"

#include "base/unguessable_token.h"
#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "net/http/http_util.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "neva/app_runtime/public/app_runtime_event.h"
#include "neva/app_runtime/webview.h"
#include "neva/app_runtime/webview_profile.h"
#include "webos/browser/webos_webview_renderer_state.h"
#include "webos/common/webos_event.h"

namespace webos {

void WebViewBase::SetFileAccessBlocked(bool blocked) {
  NOTIMPLEMENTED();
}

WebViewBase::WebViewBase(int width, int height)
    : webview_(new app_runtime::WebView(width, height)),
      injection_browser_control_handler_(
          new app_runtime::InjectionBrowserControlHandler(this)) {
  injection_browser_control_handler_->SetDelegate(this);
  webview_->SetDelegate(this);
  PushStateToIOThread();
}

WebViewBase::~WebViewBase() {
  RemoveStateFromIOThread(GetWebContents());
  webview_->SetDelegate(nullptr);
  delete webview_;
}

void WebViewBase::Initialize(const std::string& app_id,
                             const std::string& app_path,
                             const std::string& trust_level,
                             const std::string& v8_snapshot_path,
                             const std::string& v8_extra_flags,
                             bool use_native_scroll) {
  SetAppPath(app_path);
  SetTrustLevel(trust_level);
  SetAppId(app_id);
  SetV8SnapshotPath(v8_snapshot_path);
  SetV8ExtraFlags(v8_extra_flags);
  SetUseNativeScroll(use_native_scroll);

  if (GetWebContents()) {
    // The arcitecture of using WebContentsImpl is not good way and is not what
    // we want to do
    // Need to change it
    content::RenderViewHost* rvh = GetWebContents()->GetRenderViewHost();
    if (!rvh->IsRenderViewLive()) {
      // TODO(pikulik): should be revised
      content::WebContentsImpl* webcontents_impl =
          static_cast<content::WebContentsImpl*>(GetWebContents());
      webcontents_impl->CreateRenderViewForRenderManager(
          rvh, MSG_ROUTING_NONE, MSG_ROUTING_NONE,
          rvh->GetMainFrame()->GetDevToolsFrameToken(),
          content::FrameReplicationState());
    }
  }

  NOTIMPLEMENTED() << " native scrolls, allow mouse on/off event";
}

content::WebContents* WebViewBase::GetWebContents() {
  return webview_->GetWebContents();
}

void WebViewBase::AddUserStyleSheet(const std::string& sheet) {
  webview_->AddUserStyleSheet(sheet);
}

std::string WebViewBase::DefaultUserAgent() const {
  return app_runtime::GetUserAgent();
}

std::string WebViewBase::UserAgent() const {
  return webview_->UserAgent();
}

void WebViewBase::LoadUrl(const std::string& url) {
  webview_->LoadUrl(GURL(url));
}

void WebViewBase::StopLoading() {
  webview_->StopLoading();
}

void WebViewBase::LoadExtension(const std::string& name) {
  webview_->LoadExtension("v8/" + name);
}

void WebViewBase::ClearExtensions() {
  webview_->ClearExtensions();
}

void WebViewBase::EnableInspectablePage() {
  GetWebContents()->EnableInspectable();
}

void WebViewBase::SetInspectable(bool enable) {
  webview_->SetInspectable(enable);
}

void WebViewBase::AddAvailablePluginDir(const std::string& directory) {
  NOTIMPLEMENTED();
}

void WebViewBase::AddCustomPluginDir(const std::string& directory) {
  webview_->AddCustomPluginDir(directory);
}

void WebViewBase::SetUserAgent(const std::string& useragent) {
  GetWebContents()->SetUserAgentOverride(useragent, false);
}

void WebViewBase::SetBackgroundColor(int r, int g, int b, int alpha) {
  webview_->SetBackgroundColor(r, g, b, alpha);
}

void WebViewBase::SetAllowFakeBoldText(bool allow) {
  webview_->SetAllowFakeBoldText(allow);
}

void WebViewBase::SetShouldSuppressDialogs(bool suppress) {
  webview_->SetShouldSuppressDialogs(suppress);
}

void WebViewBase::SetAppId(const std::string& app_id) {
  webview_->SetAppId(app_id);
}

void WebViewBase::SetSecurityOrigin(const std::string& identifier) {
  webview_->SetSecurityOrigin(identifier);
}

void WebViewBase::SetAcceptLanguages(const std::string& languages) {
  webview_->SetAcceptLanguages(languages);
}

void WebViewBase::SetUseLaunchOptimization(bool enabled, int delay_ms) {
  webview_->SetUseLaunchOptimization(enabled, delay_ms);
}

void WebViewBase::SetUseEnyoOptimization(bool enabled) {
  webview_->SetUseEnyoOptimization(enabled);
}

void WebViewBase::SetUseAccessibility(bool enabled) {
  if (enabled)
    GetWebContents()->EnableWebContentsOnlyAccessibilityMode();
}

void WebViewBase::SetAppPreloadHint(bool is_preload) {
  webview_->SetAppPreloadHint(is_preload);
}

void WebViewBase::SetTransparentBackground(bool enable) {
  webview_->SetTransparentBackground(enable);
}

void WebViewBase::SetBoardType(const std::string& board_type) {
  webview_->SetBoardType(board_type);
}

void WebViewBase::SetMediaCodecCapability(const std::string& capability) {
  webview_->SetMediaCodecCapability(capability);
}

void WebViewBase::SetSearchKeywordForCustomPlayer(bool enabled) {
  webview_->SetSearchKeywordForCustomPlayer(enabled);
}

void WebViewBase::SetSupportDolbyHDRContents(bool support) {
  webview_->SetSupportDolbyHDRContents(support);
}

void WebViewBase::SetUseUnlimitedMediaPolicy(bool enabled) {
  webview_->SetUseUnlimitedMediaPolicy(enabled);
}

void WebViewBase::SetActiveOnNonBlankPaint(bool active) {
  webview_->SetActiveOnNonBlankPaint(active);
}

void WebViewBase::SetViewportSize(int width, int height) {
  webview_->SetViewportSize(width, height);
}

void WebViewBase::NotifyMemoryPressure(MemoryPressureLevel level) {
  base::MemoryPressureListener::MemoryPressureLevel pressure_level =
      base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_NONE;
  if (level == MemoryPressureLevel::MEMORY_PRESSURE_LOW) {
    pressure_level =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_MODERATE;
  } else if (level == MemoryPressureLevel::MEMORY_PRESSURE_CRITICAL) {
    pressure_level =
        base::MemoryPressureListener::MEMORY_PRESSURE_LEVEL_CRITICAL;
  }
  webview_->NotifyMemoryPressure(pressure_level);
}

void WebViewBase::SetVisible(bool visible) {
  webview_->SetVisible(visible);
}

void WebViewBase::SetProxyServer(const std::string& proxyIp,
                                 const std::string& proxyPort,
                                 const std::string& proxyUsername,
                                 const std::string& proxyPassword) {
  GetProfile()->SetProxyServer(proxyIp, proxyPort, proxyUsername,
                               proxyPassword);
}

app_runtime::WebPageVisibilityState WebViewBase::FromNativeVisibilityState(
  WebPageVisibilityState visibility_state) {
  switch (visibility_state) {
    case WebPageVisibilityStateVisible:
      return app_runtime::WebPageVisibilityState::WebPageVisibilityStateVisible;
    case WebPageVisibilityStateHidden:
      return app_runtime::WebPageVisibilityState::WebPageVisibilityStateHidden;
    case WebPageVisibilityStateLaunching:
      return app_runtime::WebPageVisibilityState::WebPageVisibilityStateLaunching;
    case WebPageVisibilityStatePrerender:
    default:
      return app_runtime::WebPageVisibilityState::WebPageVisibilityStatePrerender;
  }
}

void WebViewBase::SetVisibilityState(WebPageVisibilityState visibility_state) {
  webview_->SetVisibilityState(FromNativeVisibilityState(visibility_state));
}

void WebViewBase::SetPrerenderState() {
  NOTIMPLEMENTED();
}

void WebViewBase::DeleteWebStorages(const std::string& identifier) {
  webview_->DeleteWebStorages(identifier);
}

std::string WebViewBase::DocumentTitle() const {
  return webview_->DocumentTitle();
}

void WebViewBase::SuspendWebPageDOM() {
  webview_->SuspendDOM();
}

void WebViewBase::ReplaceBaseURL(const std::string& new_url,
                                 const std::string& old_url) {
  if (new_url != old_url)
    webview_->ReplaceBaseURL(GURL(new_url));
}

void WebViewBase::ResumeWebPageDOM() {
  webview_->ResumeDOM();
}

void WebViewBase::SuspendWebPageMedia() {
  webview_->SuspendMedia();
}

void WebViewBase::ResumeWebPageMedia() {
  webview_->ResumeMedia();
}

void WebViewBase::SuspendPaintingAndSetVisibilityHidden() {
  webview_->SuspendPaintingAndSetVisibilityHidden();
}

void WebViewBase::ResumePaintingAndSetVisibilityVisible() {
  webview_->ResumePaintingAndSetVisibilityVisible();
}

void WebViewBase::CommitLoadVisually() {
  webview_->CommitLoadVisually();
}

const std::string& WebViewBase::GetUrl() {
  return webview_->GetUrl();
}

void WebViewBase::RunJavaScript(const std::string& js_code) {
  webview_->RunJavaScript(js_code);
}

void WebViewBase::RunJavaScriptInAllFrames(const std::string& js_code) {
  webview_->RunJavaScriptInAllFrames(js_code);
}

void WebViewBase::Reload() {
  webview_->Reload();
}

int WebViewBase::RenderProcessPid() const {
  return webview_->RenderProcessPid();
}

bool WebViewBase::IsDrmEncrypted(const std::string& url) {
  return webview_->IsDrmEncrypted(url);
}

std::string WebViewBase::DecryptDrm(const std::string& url) {
  return webview_->DecryptDrm(url);
}

void WebViewBase::SetFocus(bool focus) {
  webview_->SetFocus(focus);
}

double WebViewBase::GetZoomFactor() {
  return webview_->GetZoomFactor();
}

void WebViewBase::SetZoomFactor(double factor) {
  webview_->SetZoomFactor(factor);
}

void WebViewBase::SetDoNotTrack(bool dnt) {
  webview_->SetDoNotTrack(dnt);
}

void WebViewBase::ForwardAppRuntimeEvent(app_runtime::AppRuntimeEvent* event) {
  webview_->ForwardAppRuntimeEvent(event);
}

void WebViewBase::ForwardWebOSEvent(WebOSEvent* event) {
  // TODO: Need refactoring
  if (event->GetType() == WebOSEvent::KeyPress ||
      event->GetType() == WebOSEvent::KeyRelease) {
    WebOSKeyEvent* key_event = static_cast<WebOSKeyEvent*>(event);
    int keycode = key_event->GetCode();
    key_event->SetCode(keycode);
  }
  webview_->ForwardAppRuntimeEvent(event);
}

bool WebViewBase::CanGoBack() const {
  return webview_->CanGoBack();
}

bool WebViewBase::IsInputMethodActive() {
  NOTIMPLEMENTED();
  return false;
}

void WebViewBase::GoBack() {
  webview_->GoBack();
}

void WebViewBase::RequestGetCookies(const std::string& url) {
  webview_->RequestGetCookies(url);
}

void WebViewBase::SetAdditionalContentsScale(float scale_x, float scale_y) {
  webview_->SetAdditionalContentsScale(scale_x, scale_y);
}

void WebViewBase::SetHardwareResolution(int width, int height) {
  webview_->SetHardwareResolution(width, height);
}

void WebViewBase::SetEnableHtmlSystemKeyboardAttr(bool enable) {
  webview_->SetEnableHtmlSystemKeyboardAttr(enable);
}

void WebViewBase::DropAllPeerConnections(DropPeerConnectionReason reason) {
  app_runtime::DropPeerConnectionReason app_runtime_reason;
  switch (reason) {
    case DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN:
      app_runtime_reason = app_runtime::DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN;
      break;
    case DROP_PEER_CONNECTION_REASON_UNKNOWN:
    default:
      app_runtime_reason = app_runtime::DROP_PEER_CONNECTION_REASON_UNKNOWN;
  }
  webview_->DropAllPeerConnections(app_runtime_reason);
}

void WebViewBase::DidDropAllPeerConnections(
    app_runtime::DropPeerConnectionReason reason) {
  DropPeerConnectionReason webos_reason;
  switch (reason) {
    case app_runtime::DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN:
      webos_reason = DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN;
      break;
    case app_runtime::DROP_PEER_CONNECTION_REASON_UNKNOWN:
    default:
      webos_reason = DROP_PEER_CONNECTION_REASON_UNKNOWN;
  }
  DidDropAllPeerConnections(webos_reason);
}

void WebViewBase::RequestInjectionLoading(const std::string& injection_name) {
  webview_->RequestInjectionLoading(injection_name);
}

// WebPreferences
void WebViewBase::SetAllowRunningInsecureContent(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::AllowRunningInsecureContent, enable);
}

void WebViewBase::SetAllowScriptsToCloseWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::AllowScriptsToCloseWindows, enable);
}

void WebViewBase::SetAllowUniversalAccessFromFileUrls(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::AllowUniversalAccessFromFileUrls,
      enable);
}

void WebViewBase::SetRequestQuotaEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::RequestQuotaEnabled,
      enable);
}

void WebViewBase::SetSuppressesIncrementalRendering(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::SuppressesIncrementalRendering, enable);
}

void WebViewBase::SetDisallowScrollbarsInMainFrame(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::DisallowScrollbarsInMainFrame, enable);
}

void WebViewBase::SetDisallowScrollingInMainFrame(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::DisallowScrollingInMainFrame, enable);
}

#if defined(HBBTV)
// Stubs for hbbtv2-pluggable
void WebViewBase::SetHbbtvEnabled(bool on) {
  NOTIMPLEMENTED();
}

void WebViewBase::SetHbbtvAdvancedAdInsertionEnabled(bool on) {
  NOTIMPLEMENTED();
}

void WebViewBase::SetHbbtvClearMediaPlayerEnabled(bool on) {
  NOTIMPLEMENTED();
}

bool WebViewBase::GetHbbtvClearMediaPlayerEnabled() {
  return false;
}
#endif

void WebViewBase::SetJavascriptCanOpenWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::JavascriptCanOpenWindows, enable);
}

void WebViewBase::SetSpatialNavigationEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::SpatialNavigationEnabled, enable);
}

void WebViewBase::SetSupportsMultipleWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::SupportsMultipleWindows, enable);
}

void WebViewBase::SetCSSNavigationEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::CSSNavigationEnabled, enable);
}

void WebViewBase::SetV8DateUseSystemLocaloffset(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::V8DateUseSystemLocaloffset, enable);
}

void WebViewBase::SetAllowLocalResourceLoad(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::AllowLocalResourceLoad, enable);
}

void WebViewBase::SetLocalStorageEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::LocalStorageEnabled,
      enable);
}

void WebViewBase::SetWebSecurityEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::WebSecurityEnabled,
      enable);
}

void WebViewBase::SetKeepAliveWebApp(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::KeepAliveWebApp,
      enable);
}

void WebViewBase::SetAdditionalFontFamilyEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::AdditionalFontFamilyEnabled, enable);
}

void WebViewBase::SetDatabaseIdentifier(const std::string& identifier) {
  webview_->SetDatabaseIdentifier(identifier);
}

void WebViewBase::SetBackHistoryAPIDisabled(const bool on) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::BackHistoryAPIDisabled, on);
}

void WebViewBase::SetForceVideoTexture(bool on) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::ForceVideoTexture, on);
}

void WebViewBase::SetNotifyFMPDirectly(const bool on) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::NotifyFMPDirectly, on);
}

void WebViewBase::SetNetworkStableTimeout(const double timeout) {
  webview_->UpdatePreferencesAttribute(
      app_runtime::WebView::Attribute::NetworkStableTimeout, timeout);
}

// FontFamily
void WebViewBase::SetStandardFontFamily(const std::string& font) {
  webview_->SetFontFamily(app_runtime::WebView::FontFamily::StandardFont, font);
}

void WebViewBase::SetFixedFontFamily(const std::string& font) {
  webview_->SetFontFamily(app_runtime::WebView::FontFamily::FixedFont, font);
}

void WebViewBase::SetSerifFontFamily(const std::string& font) {
  webview_->SetFontFamily(app_runtime::WebView::FontFamily::SerifFont, font);
}

void WebViewBase::SetSansSerifFontFamily(const std::string& font) {
  webview_->SetFontFamily(
      app_runtime::WebView::FontFamily::SansSerifFont, font);
}

void WebViewBase::SetCursiveFontFamily(const std::string& font) {
  webview_->SetFontFamily(app_runtime::WebView::FontFamily::CursiveFont, font);
}

void WebViewBase::SetFantasyFontFamily(const std::string& font) {
  webview_->SetFontFamily(app_runtime::WebView::FontFamily::FantasyFont, font);
}

void WebViewBase::SetFontHinting(WebViewBase::FontRenderParams hinting) {
  switch (hinting) {
    case WebViewBase::FontRenderParams::HINTING_NONE:
      webview_->SetFontHintingNone();
      return;
    case WebViewBase::FontRenderParams::HINTING_SLIGHT:
      webview_->SetFontHintingSlight();
      return;
    case WebViewBase::FontRenderParams::HINTING_MEDIUM:
      webview_->SetFontHintingMedium();
      return;
    case WebViewBase::FontRenderParams::HINTING_FULL:
      webview_->SetFontHintingFull();
      return;
    default:
      webview_->SetFontHintingNone();
      return;
  }
}

void WebViewBase::LoadAdditionalFont(const std::string& url,
                                     const std::string& font) {
  NOTIMPLEMENTED();
}

void WebViewBase::UpdatePreferences() {
  webview_->UpdatePreferences();
}

void WebViewBase::SetAudioGuidanceOn(bool on) {
  NOTIMPLEMENTED();
}

void WebViewBase::ResetStateToMarkNextPaintForContainer() {
  webview_->ResetStateToMarkNextPaintForContainer();
}

void WebViewBase::SetAppPath(const std::string& app_path) {
  if (app_path_ == app_path)
    return;

  app_path_ = app_path;
  PushStateToIOThread();
}

void WebViewBase::SetTrustLevel(const std::string& trust_level) {
  if (trust_level_ == trust_level)
    return;

  trust_level_ = trust_level;
  PushStateToIOThread();
}

void WebViewBase::HandleBrowserControlCommand(
    const std::string& command,
    const std::vector<std::string>& arguments) {
  NOTIMPLEMENTED();
}

void WebViewBase::HandleBrowserControlFunction(
    const std::string& command,
    const std::vector<std::string>& arguments,
    std::string* result) {
  NOTIMPLEMENTED();
}

void WebViewBase::OnBrowserControlCommand(
      const std::string& command,
      const std::vector<std::string>& arguments) {
  HandleBrowserControlCommand(command, arguments);
}

void WebViewBase::OnBrowserControlFunction(
      const std::string& command,
      const std::vector<std::string>& arguments,
      std::string* result) {
  HandleBrowserControlFunction(command, arguments, result);
}

void WebViewBase::PushStateToIOThread() {
  if (!GetWebContents())
    return;

  webos::WebOSWebViewRendererState::WebViewInfo web_view_info;
  web_view_info.app_path = app_path_;
  web_view_info.trust_level = trust_level_;
  web_view_info.accept_language = accept_language_;

  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &webos::WebOSWebViewRendererState::RegisterWebViewInfo,
          base::Unretained(webos::WebOSWebViewRendererState::GetInstance()),
          GetWebContents()->GetMainFrame()->GetProcess()->GetID(),
          GetWebContents()->GetRenderViewHost()->GetRoutingID(),
          GetWebContents()->GetMainFrame()->GetFrameTreeNodeId(),
          web_view_info));
}

void WebViewBase::RemoveStateFromIOThread(content::WebContents* web_contents) {
  content::BrowserThread::PostTask(
      content::BrowserThread::IO, FROM_HERE,
      base::Bind(
          &webos::WebOSWebViewRendererState::UnRegisterWebViewInfo,
          base::Unretained(webos::WebOSWebViewRendererState::GetInstance()),
          GetWebContents()->GetMainFrame()->GetProcess()->GetID(),
          GetWebContents()->GetRenderViewHost()->GetRoutingID()));
}

void WebViewBase::SetSSLCertErrorPolicy(
    app_runtime::SSLCertErrorPolicy policy) {
  webview_->SetSSLCertErrorPolicy(policy);
}

app_runtime::SSLCertErrorPolicy WebViewBase::GetSSLCertErrorPolicy() {
  return webview_->GetSSLCertErrorPolicy();
}

app_runtime::WebViewProfile* WebViewBase::GetProfile() const {
  return webview_->GetProfile();
}

void WebViewBase::SetProfile(app_runtime::WebViewProfile* profile) {
  webview_->SetProfile(profile);
}

void WebViewBase::SetV8SnapshotPath(const std::string& v8_snapshot_path) {
  webview_->SetV8SnapshotPath(v8_snapshot_path);
}

void WebViewBase::SetV8ExtraFlags(const std::string& v8_extra_flags) {
  webview_->SetV8ExtraFlags(v8_extra_flags);
}

void WebViewBase::SetUseNativeScroll(bool use_native_scroll) {
  webview_->SetUseNativeScroll(use_native_scroll);
}

}  // namespace webos
