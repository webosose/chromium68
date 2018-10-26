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

#include "neva/app_runtime/webview.h"

#include "base/files/file_path.h"
#include "base/strings/utf_string_conversions.h"
#include "browser/app_runtime_browser_context_adapter.h"
#include "components/media_capture_util/devices_dispatcher.h"
#include "content/browser/frame_host/render_frame_host_impl.h"
#include "content/browser/frame_host/frame_tree_node.h"
#include "content/browser/renderer_host/render_view_host_impl.h"
#include "content/browser/renderer_host/render_widget_host_view_aura.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/dom_storage_context.h"
#include "content/public/browser/favicon_status.h"
#include "content/public/browser/host_zoom_map.h"
#include "content/public/browser/invalidate_type.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/navigation_entry.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/plugin_service.h"
#include "content/public/browser/render_frame_host.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/render_widget_host_view.h"
#include "content/public/browser/resource_context.h"
#include "content/public/browser/storage_partition.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/favicon_url.h"
#include "content/public/common/page_zoom.h"
#include "content/public/common/renderer_preferences.h"
#include "content/public/common/user_agent.h"
#include "content/public/common/web_preferences.h"
#include "ipc/ipc_message_macros.h"
#include "net/base/net_errors.h"
#include "net/cookies/cookie_store.h"
#include "net/url_request/url_request_context.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"
#include "neva/app_runtime/common/app_runtime.mojom.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "neva/app_runtime/public/app_runtime_event.h"
#include "neva/app_runtime/public/webview_delegate.h"
#include "neva/app_runtime/webapp_injection_manager.h"
#include "neva/app_runtime/webview_profile.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_provider.h"
#include "third_party/blink/public/mojom/page/page_visibility_state.mojom.h"
#include "ui/aura/window.h"
#include "ui/aura/client/screen_position_client.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"
#include "ui/events/blink/web_input_event.h"
#include "ui/events/event.h"
#include "ui/events/event_constants.h"
#include "ui/events/event_utils.h"
#include "ui/gfx/font_render_params.h"

#if defined(USE_APPDRM)
#include "net/appdrm/appdrm_file_manager.h"
#endif

#if defined(ENABLE_PLUGINS)
void GetPluginsCallback(const std::vector<content::WebPluginInfo>& plugins) {
}
#endif

namespace app_runtime {

gfx::PointF GetScreenLocationFromEvent(const ui::LocatedEvent& event) {
  aura::Window* root =
      static_cast<aura::Window*>(event.target())->GetRootWindow();
  aura::client::ScreenPositionClient* spc =
      aura::client::GetScreenPositionClient(root);
  if (!spc)
    return event.root_location_f();

  gfx::PointF screen_location(event.root_location_f());
  spc->ConvertPointToScreen(root, &screen_location);
  return screen_location;
}

void WebView::SetFileAccessBlocked(bool blocked) {
  NOTIMPLEMENTED();
}

MojoAppRuntimeHostImpl::MojoAppRuntimeHostImpl(
    content::WebContents* web_contents)
    : bindings_(web_contents, this),
      first_meaningful_paint_detected_(0.),
      arriving_meaningful_paint_(0.),
      load_visually_committed_(false) {}

MojoAppRuntimeHostImpl::~MojoAppRuntimeHostImpl() = default;

void MojoAppRuntimeHostImpl::ResetStateToMarkNextPaintForContainer() {
  first_meaningful_paint_detected_ = 0;
  arriving_meaningful_paint_ = 0;
  load_visually_committed_ = false;
}

void MojoAppRuntimeHostImpl::WillSwapMeaningfulPaint(double detected_time) {
  arriving_meaningful_paint_ = detected_time;
  LoadVisuallyCommittedIfNeed();
}

void MojoAppRuntimeHostImpl::DidFirstMeaningfulPaint(double fmp_detected) {
  first_meaningful_paint_detected_ = fmp_detected;
  LoadVisuallyCommitted();
}

void MojoAppRuntimeHostImpl::DidNonFirstMeaningfulPaint() {
  LoadVisuallyCommitted();
}

void MojoAppRuntimeHostImpl::LoadVisuallyCommitted() {
  if (load_visually_committed_)
    return;
  if (web_view_delegate_) {
    web_view_delegate_->LoadVisuallyCommitted();
    load_visually_committed_ = true;
  }
}

void MojoAppRuntimeHostImpl::LoadVisuallyCommittedIfNeed() {
  if (load_visually_committed_ || first_meaningful_paint_detected_ == .0)
    return;
  if (first_meaningful_paint_detected_ <= arriving_meaningful_paint_) {
    LoadVisuallyCommitted();
    load_visually_committed_ = true;
  }
}

void MojoAppRuntimeHostImpl::DidClearWindowObject() {
  if (web_view_delegate_)
    web_view_delegate_->DidClearWindowObject();
}

WebView::WebView(int width, int height, WebViewProfile* profile)
    : should_suppress_dialogs_(false),
      active_on_non_blank_paint_(false),
      width_(width),
      height_(height),
      full_screen_(false),
      enable_skip_frame_(false),
      ssl_cert_error_policy_(SSL_CERT_ERROR_POLICY_DEFAULT),
      profile_(profile ? profile : WebViewProfile::GetDefaultProfile()) {
  if (display::Screen::GetScreen()->GetNumDisplays() > 0) {
    // WebView constructor width and height params have default values in
    // webview.h. If screen is rotated then initial size might be different
    // and default values may lead to incorrectly scaled view for the first
    // rendered frame.
    gfx::Size displaySize =
        display::Screen::GetScreen()->GetPrimaryDisplay().bounds().size();
    width_ = displaySize.width();
    height_ = displaySize.height();
  }

  CreateWebContents();
  web_contents_->SetDelegate(this);
  Observe(web_contents_.get());
  host_interface_ =
      std::make_unique<MojoAppRuntimeHostImpl>(web_contents_.get());

  // Default policy : Skip frame is enabled.
  SetSkipFrame(true);

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  rvh->SyncRendererPrefs();
  web_preferences_.reset(
      new content::WebPreferences(rvh->GetWebkitPreferences()));
}

WebView::~WebView() {
  web_contents_->SetDelegate(nullptr);
}

void WebView::SetDelegate(WebViewDelegate* delegate) {
  webview_delegate_ = delegate;
  host_interface_->SetDelegate(delegate);
}

void WebView::CreateWebContents() {
  content::BrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  content::WebContents::CreateParams params(browser_context, nullptr);
  params.routing_id = MSG_ROUTING_NONE;
  params.initial_size = gfx::Size(width_, height_);
  web_contents_ = content::WebContents::Create(params);
  injection_manager_ = std::make_unique<WebAppInjectionManager>(*web_contents_);
}

content::WebContents* WebView::GetWebContents() {
  return web_contents_.get();
}

void WebView::AddUserStyleSheet(const std::string& sheet) {
  web_contents_->InjectCSS(sheet);
}

std::string WebView::UserAgent() const {
  return web_contents_->GetUserAgentOverride();
}

void WebView::LoadUrl(const GURL& url) {
  content::NavigationController::LoadURLParams params(url);
  params.transition_type = ui::PageTransitionFromInt(
      ui::PAGE_TRANSITION_TYPED | ui::PAGE_TRANSITION_FROM_API);
  params.frame_name = std::string("");
  params.override_user_agent =
      content::NavigationController::UA_OVERRIDE_TRUE;
  web_contents_->GetController().LoadURLWithParams(params);
}

void WebView::StopLoading() {
  int index = web_contents_->GetController().GetPendingEntryIndex();
  if (index != -1)
    web_contents_->GetController().RemoveEntryAtIndex(index);

  web_contents_->Stop();
  web_contents_->Focus();
}

void WebView::LoadExtension(const std::string& name) {
  RequestInjectionLoading(name);
}

void WebView::ClearExtensions() {
  RequestClearInjections();
}

void WebView::ReplaceBaseURL(const GURL& newUrl) {
  web_contents_->ReplaceBaseURL(newUrl);
}

const std::string& WebView::GetUrl() {
  return web_contents_->GetVisibleURL().spec();
}

void WebView::SuspendDOM() {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    mojom::AppRuntimeClientAssociatedPtr client;
    rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
    client->SuspendDOM();
  }
}

void WebView::ResumeDOM() {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    mojom::AppRuntimeClientAssociatedPtr client;
    rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
    client->ResumeDOM();
  }
}

void WebView::SuspendMedia() {
#if defined(USE_NEVA_MEDIA)
  for (auto* rfh : web_contents_->GetAllFrames())
    rfh->SuspendMedia();
#endif
}

void WebView::ResumeMedia() {
#if defined(USE_NEVA_MEDIA)
  for (auto* rfh : web_contents_->GetAllFrames())
    rfh->ResumeMedia();
#endif
}

void WebView::SuspendPaintingAndSetVisibilityHidden() {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  if (host_view)
    host_view->Hide();
}

void WebView::ResumePaintingAndSetVisibilityVisible() {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  if (host_view)
    host_view->Show();
}

void WebView::EnableAggressiveReleasePolicy(bool enable) {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          web_contents_->GetRenderViewHost()->GetWidget()->GetView());
  if (host_view)
    host_view->EnableAggressiveReleasePolicy(enable);
}

bool WebView::SetSkipFrame(bool enable) {
  NOTIMPLEMENTED();
  return true;
}

void WebView::CommitLoadVisually() {
  NOTIMPLEMENTED();
}

std::string WebView::DocumentTitle() const {
  return document_title_;
}

void WebView::RunJavaScript(const std::string& jsCode) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->GetMainFrame()->Send(new FrameMsg_JavaScriptExecuteRequest(
        rvh->GetMainFrame()->GetRoutingID(), base::UTF8ToUTF16(jsCode), 0,
        false));
}

void WebView::RunJavaScriptInAllFrames(const std::string& js_code) {
  web_contents_->ExecuteJavaScriptInAllFrames(base::UTF8ToUTF16(js_code));
}

void WebView::Reload() {
  web_contents_->GetController().Reload(content::ReloadType::NONE, false);
  web_contents_->Focus();
}

int WebView::RenderProcessPid() const {
  content::RenderProcessHost* host = web_contents_->GetMainFrame()->GetProcess();
  if (host)
    return host->GetProcess().Handle();
  return -1;
}

bool WebView::IsDrmEncrypted(const std::string& url) {
#if defined(USE_APPDRM)
  net::AppDRMFileManager adfm = net::AppDRMFileManager(url);
  return adfm.Check();
#else
  return false;
#endif
}

std::string WebView::DecryptDrm(const std::string& url) {
#if defined(USE_APPDRM)
  net::AppDRMFileManager adfm = net::AppDRMFileManager(url);
  return adfm.Decrypt();
#else
  return std::string("");
#endif
}

int WebView::DevToolsPort() const {
  return static_cast<AppRuntimeBrowserMainParts*>(
      GetAppRuntimeContentBrowserClient()->GetMainParts())
      ->DevToolsPort();
}

void WebView::SetInspectable(bool enable) {
  AppRuntimeBrowserMainParts* mp =
    static_cast<AppRuntimeBrowserMainParts*>(
      GetAppRuntimeContentBrowserClient()->GetMainParts());

  if (enable)
    mp->EnableDevTools();
  else
    mp->DisableDevTools();
}

void WebView::AddCustomPluginDir(const std::string& directory) {
  NOTIMPLEMENTED();
}

void WebView::SetBackgroundColor(int r, int g, int b, int a) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    mojom::AppRuntimeClientAssociatedPtr client;
    rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
    client->SetBackgroundColor(r, g, b, a);
  }
}

void WebView::SetAllowFakeBoldText(bool allow) {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  if (renderer_prefs->allow_fake_bold_text == allow)
    return;

  renderer_prefs->allow_fake_bold_text = allow;

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::LoadProgressChanged(content::WebContents* source,
                                  double progress) {
  if (webview_delegate_)
    webview_delegate_->OnLoadProgressChanged(progress);
}

void WebView::NavigationStateChanged(content::WebContents* source,
                                     content::InvalidateTypes changed_flags) {
  if (webview_delegate_ && (content::INVALIDATE_TYPE_TITLE & changed_flags))
    webview_delegate_->TitleChanged(base::UTF16ToUTF8(source->GetTitle()));
}

void WebView::CloseContents(content::WebContents* source) {
  if (webview_delegate_)
    webview_delegate_->Close();
}

gfx::Size WebView::GetSizeForNewRenderView(
    content::WebContents* web_contents) const {
  if (web_contents_->GetContainerBounds().width() == 0
      && web_contents_->GetContainerBounds().height() == 0)
    return gfx::Size(width_, height_);
  else
    return web_contents_->GetContainerBounds().size();
}

bool WebView::ShouldSuppressDialogs(content::WebContents* source) {
  return should_suppress_dialogs_;
}

void WebView::SetShouldSuppressDialogs(bool suppress) {
  should_suppress_dialogs_ = suppress;
}

void WebView::SetAppId(const std::string& app_id) {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  if (!renderer_prefs->application_id.compare(app_id))
    return;

  renderer_prefs->application_id = app_id;

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetAcceptLanguages(const std::string& languages) {
  auto* rendererPrefs(web_contents_->GetMutableRendererPrefs());
  if (!rendererPrefs->accept_languages.compare(languages))
    return;

  rendererPrefs->accept_languages = languages;

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetUseLaunchOptimization(bool enabled, int delay_ms) {
  NOTIMPLEMENTED();
}

void WebView::SetUseEnyoOptimization(bool enabled) {
  NOTIMPLEMENTED();
  // TODO(jose.dapena): patch not ported
}

void WebView::SetAppPreloadHint(bool is_preload) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SetAppPreloadHint(is_preload);
}

void WebView::SetTransparentBackground(bool enable) {
  content::RenderWidgetHostView* const host_view =
      web_contents_->GetRenderWidgetHostView();
  if (host_view)
    host_view->SetBackgroundColor(enable ? SK_ColorTRANSPARENT : SK_ColorBLACK);
}

void WebView::SetBoardType(const std::string& board_type) {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  if (!renderer_prefs->board_type.compare(board_type))
    return;

  renderer_prefs->board_type = board_type;

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetMediaCodecCapability(const std::string& capability) {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  if (!renderer_prefs->media_codec_capability.compare(capability))
    return;

  // used by Neva project
  renderer_prefs->media_codec_capability = capability;
}

void WebView::SetSearchKeywordForCustomPlayer(bool enabled) {
  NOTIMPLEMENTED();
  // TODO(jose.dapena): patch not ported
}

void WebView::SetSupportDolbyHDRContents(bool support) {
  NOTIMPLEMENTED();
  // TODO(jose.dapena): patch not ported
}

void WebView::UpdatePreferencesAttributeForPrefs(
    content::WebPreferences* preferences,
    WebView::Attribute attribute,
    bool enable) {
  switch (attribute) {
    case Attribute::AllowRunningInsecureContent:
      preferences->allow_running_insecure_content = enable;
      break;
    case Attribute::AllowScriptsToCloseWindows:
      preferences->allow_scripts_to_close_windows = enable;
      break;
    case Attribute::AllowUniversalAccessFromFileUrls:
      preferences->allow_universal_access_from_file_urls = enable;
      break;
    case Attribute::SuppressesIncrementalRendering:
      NOTIMPLEMENTED()
          << "Attribute::SuppressesIncrementalRendering is not supported";
      return;
    case Attribute::DisallowScrollbarsInMainFrame:
      NOTIMPLEMENTED()
          << "Attribute::DisallowScrollbarsInMainFrame is not supported";
      return;
    // According commit 5c434bb2 : Remove obsolete Blink popup blocker
    // removed javascript_can_open_windows_automatically preference.
    case Attribute::SpatialNavigationEnabled:
      preferences->spatial_navigation_enabled = enable;
      break;
    case Attribute::SupportsMultipleWindows:
      preferences->supports_multiple_windows = enable;
      break;
    case Attribute::CSSNavigationEnabled:
      preferences->css_navigation_enabled = enable;
      break;
      return;
    case Attribute::AllowLocalResourceLoad:
      NOTIMPLEMENTED() << "Attribute::AllowLocalResourceLoad is not supported";
      return;
    case Attribute::LocalStorageEnabled:
      preferences->local_storage_enabled = enable;
      break;
    case Attribute::WebSecurityEnabled:
      preferences->web_security_enabled = enable;
      break;
    case Attribute::RequestQuotaEnabled:
    case Attribute::DisallowScrollingInMainFrame:
    case Attribute::V8DateUseSystemLocaloffset:
    case Attribute::KeepAliveWebApp:
    case Attribute::AdditionalFontFamilyEnabled:
      // TODO(jose.dapena): patches not ported
      NOTIMPLEMENTED() << "patches not ported";
      return;
    case Attribute::NotifyFMPDirectly:
      preferences->notify_fmp_directly = enable;
      break;
    default:
      return;
  }
}

void WebView::UpdatePreferencesAttribute(WebView::Attribute attribute,
                                         bool enable) {
  webview_preferences_list_[attribute] = enable;
  UpdatePreferencesAttributeForPrefs(web_preferences_.get(), attribute, enable);

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->UpdateWebkitPreferences(*web_preferences_);
}

void WebView::UpdatePreferencesAttribute(WebView::Attribute attribute,
                                         double value) {
  switch (attribute) {
    case Attribute::NetworkStableTimeout:
      web_preferences_->network_stable_timeout = value;
      break;
    default:
      return;
  }
}

void WebView::SetFontFamily(WebView::FontFamily font_family,
                            const std::string& font) {
  switch (font_family) {
    case FontFamily::StandardFont:
      web_preferences_->standard_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::FixedFont:
      web_preferences_->fixed_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::SerifFont:
      web_preferences_->serif_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::SansSerifFont:
      web_preferences_->sans_serif_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::CursiveFont:
      web_preferences_->cursive_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    case FontFamily::FantasyFont:
      web_preferences_->fantasy_font_family_map[content::kCommonScript] =
          base::ASCIIToUTF16(font.c_str());
      break;
    default:
      return;
  }

  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->UpdateWebkitPreferences(*web_preferences_.get());
}

void WebView::SetFontHintingNone() {
  content::RendererPreferences* renderer_prefs =
      web_contents_->GetMutableRendererPrefs();
  renderer_prefs->hinting = gfx::FontRenderParams::HINTING_NONE;
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetFontHintingSlight() {
  content::RendererPreferences* rendererPrefs =
      web_contents_->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_SLIGHT;
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetFontHintingMedium() {
  content::RendererPreferences* rendererPrefs =
      web_contents_->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_MEDIUM;
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetFontHintingFull() {
  content::RendererPreferences* rendererPrefs =
      web_contents_->GetMutableRendererPrefs();
  rendererPrefs->hinting = gfx::FontRenderParams::HINTING_FULL;
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh)
    rvh->SyncRendererPrefs();
}

void WebView::SetActiveOnNonBlankPaint(bool active) {
  active_on_non_blank_paint_ = active;
}

void WebView::SetViewportSize(int width, int height) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    mojom::AppRuntimeClientAssociatedPtr client;
    rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
    client->SetViewportSize(width, height);
  }
}

void WebView::NotifyMemoryPressure(
    base::MemoryPressureListener::MemoryPressureLevel level) {
  NOTIMPLEMENTED();
}

void WebView::SetVisible(bool visible) {
  if (visible)
    web_contents_->WasShown();
  else
    web_contents_->WasHidden();
}

void WebView::SetDatabaseIdentifier(const std::string& identifier) {
  NOTIMPLEMENTED();
  // TODO(jose.dapena): patch not ported
}

bool WebView::ConvertVisibilityState(WebPageVisibilityState from,
                                     blink::mojom::PageVisibilityState& to) {
    switch (from) {
      case WebPageVisibilityStateVisible:
        to = blink::mojom::PageVisibilityState::kVisible;
        break;
      case WebPageVisibilityStateHidden:
        to = blink::mojom::PageVisibilityState::kHidden;
        break;
      case WebPageVisibilityStateLaunching:
        to = blink::mojom::PageVisibilityState::kLaunching;
        break;
      case WebPageVisibilityStatePrerender:
        to = blink::mojom::PageVisibilityState::kPrerender;
        break;
      default:
        return false;
    }
    return true;
}

void WebView::SetVisibilityState(WebPageVisibilityState visibility_state) {
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (!rvh)
    return;

  blink::mojom::PageVisibilityState blink_visibility_state =
    blink::mojom::PageVisibilityState::kHidden;
  if (!ConvertVisibilityState(visibility_state, blink_visibility_state))
    return;

  mojom::AppRuntimeClientAssociatedPtr client;
  rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
  client->SetVisibilityState(blink_visibility_state);
}

void WebView::DeleteWebStorages(const std::string& identifier) {
  content::BrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  content::StoragePartition* storage_partition =
      content::BrowserContext::GetStoragePartition(browser_context, nullptr);
  std::string origin = std::string("file://").append(identifier);
  storage_partition->GetDOMStorageContext()->DeleteLocalStorage(
            GURL(origin), base::DoNothing());

}

void WebView::SetFocus(bool focus) {
  if (focus) {
    web_contents_->Focus();
  }

  content::RenderWidgetHost* const rwh =
      web_contents_->GetRenderViewHost()->GetWidget();

  if (rwh) {
    if (focus)
      rwh->Focus();
    else
      rwh->Blur();
  }
}

double WebView::GetZoomFactor() {
  return content::ZoomLevelToZoomFactor(
      content::HostZoomMap::GetZoomLevel(web_contents_.get()));
}

void WebView::SetZoomFactor(double factor) {
  content::HostZoomMap::SetZoomLevel(web_contents_.get(),
                                     content::ZoomFactorToZoomLevel(factor));
}

void WebView::SetDoNotTrack(bool dnt) {
  GetAppRuntimeContentBrowserClient()->SetDoNotTrack(dnt);
}

void WebView::ForwardAppRuntimeEvent(AppRuntimeEvent* event) {
  content::RenderWidgetHostView* rwhv =
      web_contents_->GetRenderWidgetHostView();
  if (!rwhv)
    return;
  content::RenderWidgetHost* rwh = rwhv->GetRenderWidgetHost();
  if (!rwh)
    return;

  switch (event->GetType()) {
    case AppRuntimeEvent::MouseButtonRelease: {
      AppRuntimeMouseEvent* app_runtime_event =
          static_cast<AppRuntimeMouseEvent*>(event);
      ui::MouseEvent mouse_event = ui::MouseEvent(
          ui::ET_MOUSE_RELEASED,
          gfx::Point(app_runtime_event->GetX(), app_runtime_event->GetY()),
          gfx::Point(app_runtime_event->GetX(), app_runtime_event->GetY()),
          ui::EventTimeForNow(), app_runtime_event->GetFlags(), 0);

      blink::WebMouseEvent released_event = ui::MakeWebMouseEvent(
          mouse_event, base::Bind(&GetScreenLocationFromEvent));

      rwh->ForwardMouseEvent(released_event);
      break;
    }
    case AppRuntimeEvent::MouseMove: {
      AppRuntimeMouseEvent* app_runtime_event =
          static_cast<AppRuntimeMouseEvent*>(event);
      ui::MouseEvent mouse_event = ui::MouseEvent(
          ui::ET_MOUSE_MOVED,
          gfx::Point(app_runtime_event->GetX(), app_runtime_event->GetY()),
          gfx::Point(app_runtime_event->GetX(), app_runtime_event->GetY()),
          ui::EventTimeForNow(), app_runtime_event->GetFlags(), 0);

      blink::WebMouseEvent moved_event = ui::MakeWebMouseEvent(
          mouse_event, base::Bind(&GetScreenLocationFromEvent));

      rwh->ForwardMouseEvent(moved_event);
      break;
    }
    case AppRuntimeEvent::KeyPress:
    case AppRuntimeEvent::KeyRelease: {
      AppRuntimeKeyEvent* key_event = static_cast<AppRuntimeKeyEvent*>(event);
      int keycode = key_event->GetCode();

      content::NativeWebKeyboardEvent native_event(
          ui::KeyEvent(event->GetType() == AppRuntimeKeyEvent::KeyPress
                           ? ui::ET_KEY_PRESSED
                           : ui::ET_KEY_RELEASED,
                       ui::KeyboardCode(keycode), key_event->GetFlags()),
          wchar_t(keycode));

      native_event.windows_key_code = keycode;
      native_event.native_key_code = keycode;
      native_event.text[0] = 0;
      native_event.unmodified_text[0] = 0;
      native_event.SetType(event->GetType() == AppRuntimeKeyEvent::KeyPress
                               ? blink::WebInputEvent::Type::kKeyDown
                               : blink::WebInputEvent::Type::kKeyUp);
      rwh->ForwardKeyboardEvent(native_event);
      break;
    }
    default:
      break;
  }
}

bool WebView::CanGoBack() const {
  return web_contents_->GetController().CanGoBack();
}

void WebView::GoBack() {
  web_contents_->GetController().GoToOffset(-1);
  web_contents_->Focus();
}

void WebView::SendGetCookiesResponse(const net::CookieList& cookie_list) {
  std::string cookie_line =
            net::CanonicalCookie::BuildCookieLine(cookie_list);
  if (webview_delegate_)
    webview_delegate_->SendCookiesForHostname(cookie_line);

}

void WebView::RequestGetCookies(const std::string& url) {
  content::BrowserContext* browser_context =
      profile_->GetBrowserContextAdapter()->GetBrowserContext();
  net::CookieStore* cookie_store =
    browser_context->GetResourceContext()->GetRequestContext()->cookie_store();

  if (cookie_store) {
    net::CookieOptions opt;
    opt.set_include_httponly();

    cookie_store->GetCookieListWithOptionsAsync(
        GURL(url), opt, base::BindOnce(&WebView::SendGetCookiesResponse, base::Unretained(this)));
  }
}

void WebView::SetAdditionalContentsScale(float scale_x, float scale_y) {
  NOTIMPLEMENTED();
}

void WebView::SetHardwareResolution(int width, int height) {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          web_contents_->GetRenderViewHost()->GetWidget()->GetView());

 if (!host_view)
   return;

  host_view->SetHardwareResolution(width, height);
}

void WebView::SetEnableHtmlSystemKeyboardAttr(bool enable) {
  content::RenderWidgetHostViewAura* const host_view =
      static_cast<content::RenderWidgetHostViewAura*>(
          web_contents_->GetRenderViewHost()->GetWidget()->GetView());

  if (!host_view)
    return;

  host_view->SetEnableHtmlSystemKeyboardAttr(enable);
}

void WebView::RequestInjectionLoading(const std::string& injection_name) {
  injection_manager_->RequestLoadExtension(injection_name);
}

void WebView::RequestClearInjections() {
  injection_manager_->RequestClearExtensions();
}


void WebView::ResetStateToMarkNextPaintForContainer() {
  host_interface_->ResetStateToMarkNextPaintForContainer();
  content::RenderViewHost* rvh = web_contents_->GetRenderViewHost();
  if (rvh) {
    mojom::AppRuntimeClientAssociatedPtr client;
    rvh->GetMainFrame()->GetRemoteAssociatedInterfaces()->GetInterface(&client);
    client->ResetStateToMarkNextPaintForContainer();
  }
}

//////////////////////////////////////////////////////////////////////////////
// WebView, content::WebContentsObserver implementation:

void WebView::RenderViewCreated(content::RenderViewHost* render_view_host) {
  SetSkipFrame(enable_skip_frame_);
}

void WebView::DidStartLoading() {
  if (webview_delegate_)
    webview_delegate_->LoadStarted();
}

void WebView::DidFinishLoad(content::RenderFrameHost* render_frame_host,
                            const GURL& validated_url) {
#if defined(ENABLE_PLUGINS)
  if (!GetAppRuntimeContentBrowserClient()->PluginLoaded()) {
    GetAppRuntimeContentBrowserClient()->SetPluginLoaded(true);
    content::PluginService::GetInstance()->GetPlugins(
        base::Bind(&GetPluginsCallback));
  }
#endif
  std::string url = validated_url.GetContent();
  if (webview_delegate_)
    webview_delegate_->LoadFinished(url);
}

void WebView::DidUpdateFaviconURL(
    const std::vector<content::FaviconURL>& candidates) {
  for (auto& candidate : candidates) {
    if (candidate.icon_type == content::FaviconURL::IconType::kFavicon &&
        !candidate.icon_url.is_empty()) {
      content::NavigationEntry* entry =
          web_contents()->GetController().GetActiveEntry();
      if (!entry)
        continue;
      content::FaviconStatus& favicon = entry->GetFavicon();
      favicon.url = candidate.icon_url;
      favicon.valid = favicon.url.is_valid();
      break;
    }
  }
}

void WebView::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  if (!navigation_handle)
    return;

  if (navigation_handle->GetNetErrorCode() != net::OK) {
    DidFailLoad(nullptr, navigation_handle->GetURL(),
                navigation_handle->GetNetErrorCode(), base::ASCIIToUTF16(""));
    return;
  }
  if (navigation_handle->IsInMainFrame() && webview_delegate_) {
    webview_delegate_->NavigationHistoryChanged();
  }
}

void WebView::DidFailLoad(content::RenderFrameHost* render_frame_host,
                          const GURL& validated_url,
                          int error_code,
                          const base::string16& error_description) {
  std::string url = validated_url.GetContent();
  if (webview_delegate_) {
    if (error_code == net::ERR_ABORTED)
      webview_delegate_->LoadStopped(url);
    else
      webview_delegate_->LoadFailed(url, error_code,
                             base::UTF16ToUTF8(error_description));
  }
}

void WebView::RenderProcessCreated(base::ProcessHandle handle) {
  if (webview_delegate_)
    webview_delegate_->RenderProcessCreated(handle);
}

void WebView::RenderProcessGone(base::TerminationStatus status) {
  if (webview_delegate_)
    webview_delegate_->RenderProcessGone();
}

void WebView::DocumentLoadedInFrame(
    content::RenderFrameHost* render_frame_host) {
  // TODO(pikulik): Should be revised!
  if (webview_delegate_ &&
      static_cast<content::RenderFrameHostImpl*>(render_frame_host)
          ->frame_tree_node()
          ->IsMainFrame())
    webview_delegate_->DocumentLoadFinished();
}

void WebView::DidReceiveCompositorFrame() {
  if (webview_delegate_)
    webview_delegate_->DidSwapCompositorFrame();
}

void WebView::WillSwapMeaningfulPaint(double detected_time) {
  host_interface_->WillSwapMeaningfulPaint(detected_time);
}

void WebView::TitleWasSet(content::NavigationEntry* entry) {
  document_title_ = base::UTF16ToUTF8(entry->GetTitle());
}

void WebView::LoadingStateChanged(content::WebContents*,
                                  bool to_different_document) {
  if (to_different_document)
    document_title_.clear();
}

bool WebView::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(WebView, message)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void WebView::DidFrameFocused() {
  if (webview_delegate_)
    webview_delegate_->DidFirstFrameFocused();
}

void WebView::UpdatePreferences() {
  content::RenderViewHost* rvh = web_contents()->GetRenderViewHost();
  DCHECK(rvh != nullptr);
  rvh->SyncRendererPrefs();
  rvh->UpdateWebkitPreferences(*web_preferences_.get());
}

void WebView::EnterFullscreenModeForTab(content::WebContents* web_contents,
                                        const GURL& origin,
                                        const blink::WebFullscreenOptions& options) {
  full_screen_ = true;
  NotifyRenderWidgetWasResized();
}

void WebView::ExitFullscreenModeForTab(content::WebContents* web_contents) {
  full_screen_ = false;
  NotifyRenderWidgetWasResized();
}

bool WebView::IsFullscreenForTabOrPending(
    const content::WebContents* web_contents) const {
  return full_screen_;
}

void WebView::NotifyRenderWidgetWasResized() {
  content::RenderViewHost* rvh = web_contents()->GetRenderViewHost();
  if (!rvh)
    return;
  content::RenderWidgetHost* rwh = rvh->GetWidget();
  if (rwh)
    rwh->SynchronizeVisualProperties();
}

bool WebView::CheckMediaAccessPermission(content::RenderFrameHost* render_frame_host,
                                         const GURL& security_origin,
                                         content::MediaStreamType type) {
  if (!webview_delegate_)
    return false;

  switch (type) {
    case content::MEDIA_DEVICE_AUDIO_CAPTURE:
      return webview_delegate_->AcceptsAudioCapture();
    case content::MEDIA_DEVICE_VIDEO_CAPTURE:
      return webview_delegate_->AcceptsVideoCapture();
    default:
      break;
  }
  return false;
}

void WebView::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback) {
  media_capture_util::DevicesDispatcher::GetInstance()
      ->ProcessMediaAccessRequest(
          web_contents, request, webview_delegate_->AcceptsVideoCapture(),
          webview_delegate_->AcceptsAudioCapture(), callback);
}

bool WebView::DecidePolicyForResponse(bool isMainFrame,
                                      int statusCode,
                                      const GURL& url,
                                      const base::string16& statusText) {
  if (webview_delegate_)
    return webview_delegate_->DecidePolicyForResponse(
        isMainFrame, statusCode, url.spec(), base::UTF16ToUTF8(statusText));

  return false;
}

WebViewProfile* WebView::GetProfile() const {
  return profile_;
}

void WebView::SetProfile(WebViewProfile* profile) {
  // FIXME: Possible memory leak. We need to destroy previous profile if
  // it's not default one. Default profile is shared between all webview.
  profile_ = profile;
}

void WebView::OverrideWebkitPrefs(content::WebPreferences* prefs) {
  if (!web_preferences_)
    return;

  for (const auto& preference : webview_preferences_list_)
    UpdatePreferencesAttributeForPrefs(prefs, preference.first,
                                       preference.second);

  // Sync Fonts
  prefs->standard_font_family_map[content::kCommonScript] =
      web_preferences_->standard_font_family_map[content::kCommonScript];
  prefs->fixed_font_family_map[content::kCommonScript] =
      web_preferences_->fixed_font_family_map[content::kCommonScript];
  prefs->serif_font_family_map[content::kCommonScript] =
      web_preferences_->serif_font_family_map[content::kCommonScript];
  prefs->sans_serif_font_family_map[content::kCommonScript] =
      web_preferences_->sans_serif_font_family_map[content::kCommonScript];
  prefs->cursive_font_family_map[content::kCommonScript] =
      web_preferences_->cursive_font_family_map[content::kCommonScript];
  prefs->fantasy_font_family_map[content::kCommonScript] =
      web_preferences_->fantasy_font_family_map[content::kCommonScript];
}

void WebView::SetV8SnapshotPath(const std::string& v8_snapshot_path) {
  GetAppRuntimeContentBrowserClient()->SetV8SnapshotPath(
      web_contents_->GetMainFrame()->GetProcess()->GetID(), v8_snapshot_path);
}

void WebView::SetV8ExtraFlags(const std::string& v8_extra_flags) {
  GetAppRuntimeContentBrowserClient()->SetV8ExtraFlags(
      web_contents_->GetMainFrame()->GetProcess()->GetID(), v8_extra_flags);
}

}  // namespace app_runtime
