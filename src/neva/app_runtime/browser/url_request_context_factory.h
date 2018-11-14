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

#ifndef NEVA_APP_RUNTIME_BROWSER_URL_REQUEST_CONTEXT_FACTORY_H_
#define NEVA_APP_RUNTIME_BROWSER_URL_REQUEST_CONTEXT_FACTORY_H_

#include "content/public/browser/browser_context.h"
#include "content/public/browser/content_browser_client.h"
#include "net/http/http_network_session.h"

namespace net {
class CodeCache;
class CookieStore;
class CTPolicyEnforcer;
class MultiLogCTVerifier;
class HttpTransactionFactory;
class HttpUserAgentSettings;
class NetworkDelegate;
class URLRequestJobFactory;
}  // namespace net

namespace app_runtime {

class URLRequestContextFactory {
 public:
  explicit URLRequestContextFactory(net::NetworkDelegate* delegate);
  ~URLRequestContextFactory();

  // Some members must be initialized on UI thread.
  void InitializeOnUIThread();

  // Since main context requires a bunch of input params, if these get called
  // multiple times, either multiple main contexts should be supported/managed
  // or the input params need to be the same as before.
  // As there are multiple main contexts (each for its own profile),
  // the CreateMainGetter function return the same instance each time the methods are called
  // The media and system getters however, do not need input, so it is actually
  // safe to call these multiple times.  The impl create only 1 getter of each
  // type and return the same instance each time the methods are called, thus
  // the name difference.
  net::URLRequestContextGetter* GetSystemGetter();
  net::URLRequestContextGetter* CreateMainGetter(
      content::BrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);
  net::URLRequestContextGetter* GetMainGetter();
  net::URLRequestContextGetter* GetMediaGetter();

  void SetProxyServer(const std::string& ip,
                      const std::string& port,
                      const std::string& name,
                      const std::string& password);

 private:
  class URLRequestContextGetter;
  class MainURLRequestContextGetter;
  friend class URLRequestContextGetter;
  friend class MainURLRequestContextGetter;

  void InitializeSystemContextDependencies();
  void InitializeMainContextDependencies(
      net::HttpTransactionFactory* factory,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);
  void InitializeMediaContextDependencies(net::HttpTransactionFactory* factory);

  void PopulateNetworkSessionParams(bool ignore_certificate_errors,
                                    net::HttpNetworkSession::Params* params);
  void PopulateNetworkSessionContext(net::HttpNetworkSession::Context* context);

  // These are called by the RequestContextGetters to create each
  // RequestContext.
  // They must be called on the IO thread.
  net::URLRequestContext* CreateSystemRequestContext();
  net::URLRequestContext* CreateMediaRequestContext();
  net::URLRequestContext* CreateMainRequestContext(
      content::BrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors);

  scoped_refptr<net::URLRequestContextGetter> system_getter_;
  scoped_refptr<net::URLRequestContextGetter> media_getter_;
  scoped_refptr<net::URLRequestContextGetter> main_getter_;

  // Shared objects for all contexts.
  // The URLRequestContextStorage class is not used as owner to these objects
  // since they are shared between the different URLRequestContexts.
  // The URLRequestContextStorage class manages dependent resources for a single
  // instance of URLRequestContext only.
  bool system_dependencies_initialized_;
  std::unique_ptr<net::HostResolver> host_resolver_;
  std::unique_ptr<net::ChannelIDService> channel_id_service_;
  std::unique_ptr<net::CertVerifier> cert_verifier_;
  std::unique_ptr<net::MultiLogCTVerifier> cert_transparency_verifier_;
  std::unique_ptr<net::CTPolicyEnforcer> ct_policy_enforcer_;
  scoped_refptr<net::SSLConfigService> ssl_config_service_;
  std::unique_ptr<net::TransportSecurityState> transport_security_state_;
  std::unique_ptr<net::ProxyResolutionService> proxy_resolution_service_;
  std::unique_ptr<net::HttpAuthHandlerFactory> http_auth_handler_factory_;
  std::unique_ptr<net::HttpServerProperties> http_server_properties_;
  std::unique_ptr<net::HttpUserAgentSettings> http_user_agent_settings_;
  std::unique_ptr<net::HttpTransactionFactory> system_transaction_factory_;

  bool main_dependencies_initialized_;
  std::unique_ptr<net::HttpTransactionFactory> main_transaction_factory_;
  std::unique_ptr<net::URLRequestJobFactory> main_job_factory_;

  bool media_dependencies_initialized_;
  std::unique_ptr<net::HttpTransactionFactory> media_transaction_factory_;
  std::unique_ptr<net::CodeCache> code_cache_;

  std::unique_ptr<net::NetworkDelegate> network_delegate_;
  std::unique_ptr<net::CookieStore> cookie_store_;
  std::unique_ptr<net::HttpNetworkSession> network_session_;

#if defined(ENABLE_EMMC_OPTIMIZATIONS)
  static const int64_t kCacheMinContentLength = 16 * 1024;
#endif
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_URL_REQUEST_CONTEXT_FACTORY_H_
