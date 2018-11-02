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

#include "neva/app_runtime/public/webview_base.h"

#include "content/browser/web_contents/web_contents_impl.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "neva/app_runtime/webview.h"
#include "neva/app_runtime/webview_profile.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "neva/app_runtime/public/app_runtime_event.h"

namespace app_runtime {

void WebViewBase::SetFileAccessBlocked(bool blocked) {
  WebView::SetFileAccessBlocked(blocked);
}

WebViewBase::WebViewBase(int width, int height, WebViewProfile* profile)
    : webview_(new WebView(width, height, profile)) {
  webview_->SetDelegate(this);

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
}

WebViewBase::~WebViewBase() {
  delete webview_;
}

content::WebContents* WebViewBase::GetWebContents() {
  return webview_->GetWebContents();
}

void WebViewBase::AddUserStyleSheet(const std::string& sheet) {
  webview_->AddUserStyleSheet(sheet);
}

std::string WebViewBase::DefaultUserAgent() const {
  return GetUserAgent();
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
  webview_->LoadExtension(name);
}

void WebViewBase::EnableInspectablePage() {
  GetWebContents()->EnableInspectable();
}

int WebViewBase::DevToolsPort() const {
  return webview_->DevToolsPort();
}

void WebViewBase::SetInspectable(bool enable) {
  webview_->SetInspectable(enable);
}

void WebViewBase::AddAvailablePluginDir(const std::string& directory) {
  NOTIMPLEMENTED();
  // GetWebContents()->AddAvailablePluginDir(directory.c_str());
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

void WebViewBase::SetVisibilityState(WebPageVisibilityState visibility_state) {
  webview_->SetVisibilityState(visibility_state);
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

void WebViewBase::EnableAggressiveReleasePolicy(bool enable) {
  webview_->EnableAggressiveReleasePolicy(enable);
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

void WebViewBase::ForwardAppRuntimeEvent(AppRuntimeEvent* event) {
  webview_->ForwardAppRuntimeEvent(event);
}

bool WebViewBase::CanGoBack() const {
  return webview_->CanGoBack();
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

void WebViewBase::RequestInjectionLoading(const std::string& injection_name) {
  webview_->RequestInjectionLoading(injection_name);
}

void WebViewBase::RequestClearInjections() {
  webview_->RequestClearInjections();
}

// WebPreferences
void WebViewBase::SetAllowRunningInsecureContent(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::AllowRunningInsecureContent, enable);
}

void WebViewBase::SetAllowScriptsToCloseWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::AllowScriptsToCloseWindows, enable);
}

void WebViewBase::SetAllowUniversalAccessFromFileUrls(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::AllowUniversalAccessFromFileUrls, enable);
}

void WebViewBase::SetRequestQuotaEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(WebView::Attribute::RequestQuotaEnabled,
                                       enable);
}

void WebViewBase::SetSuppressesIncrementalRendering(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::SuppressesIncrementalRendering, enable);
}

void WebViewBase::SetDisallowScrollbarsInMainFrame(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::DisallowScrollbarsInMainFrame, enable);
}

void WebViewBase::SetDisallowScrollingInMainFrame(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::DisallowScrollingInMainFrame, enable);
}

void WebViewBase::SetJavascriptCanOpenWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::JavascriptCanOpenWindows, enable);
}

void WebViewBase::SetSpatialNavigationEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::SpatialNavigationEnabled, enable);
}

void WebViewBase::SetSupportsMultipleWindows(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::SupportsMultipleWindows, enable);
}

void WebViewBase::SetCSSNavigationEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::CSSNavigationEnabled, enable);
}

void WebViewBase::SetV8DateUseSystemLocaloffset(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::V8DateUseSystemLocaloffset, enable);
}

void WebViewBase::SetAllowLocalResourceLoad(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::AllowLocalResourceLoad, enable);
}

void WebViewBase::SetLocalStorageEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(WebView::Attribute::LocalStorageEnabled,
                                       enable);
}

void WebViewBase::SetWebSecurityEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(WebView::Attribute::WebSecurityEnabled,
                                       enable);
}

void WebViewBase::SetKeepAliveWebApp(bool enable) {
  webview_->UpdatePreferencesAttribute(WebView::Attribute::KeepAliveWebApp,
                                       enable);
}

void WebViewBase::SetAdditionalFontFamilyEnabled(bool enable) {
  webview_->UpdatePreferencesAttribute(
      WebView::Attribute::AdditionalFontFamilyEnabled, enable);
}

void WebViewBase::SetDatabaseIdentifier(const std::string& identifier) {
  webview_->SetDatabaseIdentifier(identifier);
}

// FontFamily
void WebViewBase::SetStandardFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::StandardFont, font);
}

void WebViewBase::SetFixedFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::FixedFont, font);
}

void WebViewBase::SetSerifFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::SerifFont, font);
}

void WebViewBase::SetSansSerifFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::SansSerifFont, font);
}

void WebViewBase::SetCursiveFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::CursiveFont, font);
}

void WebViewBase::SetFantasyFontFamily(const std::string& font) {
  webview_->SetFontFamily(WebView::FontFamily::FantasyFont, font);
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

void WebViewBase::SetSSLCertErrorPolicy(SSLCertErrorPolicy policy) {
  webview_->SetSSLCertErrorPolicy(policy);
}

SSLCertErrorPolicy WebViewBase::GetSSLCertErrorPolicy() {
  return webview_->GetSSLCertErrorPolicy();
}

WebViewProfile* WebViewBase::GetProfile() const {
  return webview_->GetProfile();
}

void WebViewBase::SetProfile(WebViewProfile* profile) {
  webview_->SetProfile(profile);
}

}  // namespace app_runtime
