// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_SHELL_BROWSER_SHELL_CONTENT_BROWSER_CLIENT_H_
#define EXTENSIONS_SHELL_BROWSER_SHELL_CONTENT_BROWSER_CLIENT_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "content/public/browser/certificate_request_result_type.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/resource_type.h"

class GURL;

namespace base {
class CommandLine;
}

namespace content {
class BrowserContext;
class WebContents;
}

namespace extensions {
class Extension;
class ShellBrowserMainDelegate;
class ShellBrowserMainParts;

// Content module browser process support for app_shell.
class ShellContentBrowserClient : public content::ContentBrowserClient {
 public:
  explicit ShellContentBrowserClient(
      ShellBrowserMainDelegate* browser_main_delegate);
  ~ShellContentBrowserClient() override;

  // Returns the single instance.
  static ShellContentBrowserClient* Get();

  // Returns the single browser context for app_shell.
  content::BrowserContext* GetBrowserContext();

  // content::ContentBrowserClient overrides.
  content::BrowserMainParts* CreateBrowserMainParts(
      const content::MainFunctionParams& parameters) override;
  void RenderProcessWillLaunch(
      content::RenderProcessHost* host,
      service_manager::mojom::ServiceRequest* service_request) override;
  bool ShouldUseProcessPerSite(content::BrowserContext* browser_context,
                               const GURL& effective_url) override;
  void GetQuotaSettings(
      content::BrowserContext* context,
      content::StoragePartition* partition,
      storage::OptionalQuotaSettingsCallback callback) override;
  bool IsHandledURL(const GURL& url) override;
  void SiteInstanceGotProcess(content::SiteInstance* site_instance) override;
  void SiteInstanceDeleting(content::SiteInstance* site_instance) override;
  void AppendExtraCommandLineSwitches(base::CommandLine* command_line,
                                      int child_process_id) override;
  content::SpeechRecognitionManagerDelegate*
  CreateSpeechRecognitionManagerDelegate() override;
  content::BrowserPpapiHost* GetExternalBrowserPpapiHost(
      int plugin_process_id) override;
  void GetAdditionalAllowedSchemesForFileSystem(
      std::vector<std::string>* additional_schemes) override;
  content::DevToolsManagerDelegate* GetDevToolsManagerDelegate() override;
  std::vector<std::unique_ptr<content::NavigationThrottle>>
  CreateThrottlesForNavigation(
      content::NavigationHandle* navigation_handle) override;
  std::unique_ptr<content::NavigationUIData> GetNavigationUIData(
      content::NavigationHandle* navigation_handle) override;
  void RegisterNonNetworkNavigationURLLoaderFactories(
      int frame_tree_node_id,
      NonNetworkURLLoaderFactoryMap* factories) override;
  void RegisterNonNetworkSubresourceURLLoaderFactories(
      int render_process_id,
      int render_frame_id,
      NonNetworkURLLoaderFactoryMap* factories) override;
  bool WillCreateURLLoaderFactory(
      content::RenderFrameHost* frame_host,
      bool is_navigation,
      network::mojom::URLLoaderFactoryRequest* factory_request) override;
  bool HandleExternalProtocol(
      const GURL& url,
      content::ResourceRequestInfo::WebContentsGetter web_contents_getter,
      int child_id,
      content::NavigationUIData* navigation_data,
      bool is_main_frame,
      ui::PageTransition page_transition,
      bool has_user_gesture) override;
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

 protected:
  // Subclasses may wish to provide their own ShellBrowserMainParts.
  virtual ShellBrowserMainParts* CreateShellBrowserMainParts(
      const content::MainFunctionParams& parameters,
      ShellBrowserMainDelegate* browser_main_delegate);

 private:
  // Appends command line switches for a renderer process.
  void AppendRendererSwitches(base::CommandLine* command_line);

  // Returns the extension or app associated with |site_instance| or NULL.
  const Extension* GetExtension(content::SiteInstance* site_instance);

  // Owned by content::BrowserMainLoop.
  ShellBrowserMainParts* browser_main_parts_;

  // Owned by ShellBrowserMainParts.
  ShellBrowserMainDelegate* browser_main_delegate_;

#if defined(USE_NEVA_APPRUNTIME)
  // Store the path of V8 snapshot blob for app_shell.
  std::pair<int, std::string> v8_snapshot_path_;
#endif

  DISALLOW_COPY_AND_ASSIGN(ShellContentBrowserClient);
};

}  // namespace extensions

#endif  // EXTENSIONS_SHELL_BROWSER_SHELL_CONTENT_BROWSER_CLIENT_H_
