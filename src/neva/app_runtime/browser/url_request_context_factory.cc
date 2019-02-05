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

#include "neva/app_runtime/browser/url_request_context_factory.h"

#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/lazy_instance.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "components/certificate_transparency/ct_known_logs.h"
#include "components/cookie_config/cookie_store_util.h"
#include "components/network_session_configurator/common/network_switches.h"
#include "components/os_crypt/os_crypt.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/cookie_store_factory.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/url_constants.h"
#include "net/base/proxy_server.h"
#include "net/cert/cert_verifier.h"
#include "net/cert/ct_log_verifier.h"
#include "net/cert/ct_policy_enforcer.h"
#include "net/cert/multi_log_ct_verifier.h"
#include "net/cert_net/nss_ocsp.h"
#include "net/code_cache/code_cache_impl.h"
#include "net/code_cache/dummy_code_cache.h"
#include "net/cookies/cookie_store.h"
#include "net/dns/host_resolver.h"
#include "net/http/http_auth_handler_factory.h"
#include "net/http/http_cache.h"
#include "net/http/http_network_layer.h"
#include "net/http/http_server_properties_impl.h"
#include "net/http/http_stream_factory.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#include "net/proxy_resolution/proxy_config_with_annotation.h"
#include "net/proxy_resolution/proxy_resolution_service.h"
#include "net/socket/next_proto.h"
#include "net/ssl/channel_id_service.h"
#include "net/ssl/default_channel_id_store.h"
#include "net/ssl/ssl_config_service_defaults.h"
#include "net/url_request/data_protocol_handler.h"
#include "net/url_request/file_protocol_handler.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_getter.h"
#include "net/url_request/url_request_intercepting_job_factory.h"
#include "net/url_request/url_request_job_factory_impl.h"
#include "neva/app_runtime/browser/app_runtime_browser_switches.h"
#include "neva/app_runtime/browser/app_runtime_http_user_agent_settings.h"
#include "neva/app_runtime/browser/net/app_runtime_network_delegate.h"

#include <memory>

using content::BrowserThread;

namespace app_runtime {

namespace {

const char kCacheStoreFile[] = "Cache";
const char kCookieStoreFile[] = "Cookies";
const int kDefaultDiskCacheSize = 16 * 1024 * 1024;  // default size is 16MB

}  // namespace

// Private classes to expose URLRequestContextGetter that call back to the
// URLRequestContextFactory to create the URLRequestContext on demand.
//
// The URLRequestContextFactory::URLRequestContextGetter class is used for both
// the system and media URLRequestCotnexts.
class URLRequestContextFactory::URLRequestContextGetter
    : public net::URLRequestContextGetter {
 public:
  URLRequestContextGetter(URLRequestContextFactory* factory, bool is_media)
      : is_media_(is_media), factory_(factory) {}

  net::URLRequestContext* GetURLRequestContext() override {
    if (!request_context_) {
      if (is_media_) {
        request_context_.reset(factory_->CreateMediaRequestContext());
      } else {
        request_context_.reset(factory_->CreateSystemRequestContext());
        // Set request context used by NSS for Crl requests.
        // net::SetURLRequestContextForNSSHttpIO(request_context_.get());
      }
    }
    return request_context_.get();
  }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO);
  }

 private:
  ~URLRequestContextGetter() override {}

  const bool is_media_;
  URLRequestContextFactory* const factory_;
  std::unique_ptr<net::URLRequestContext> request_context_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestContextGetter);
};

// The URLRequestContextFactory::MainURLRequestContextGetter class is used for
// the main URLRequestContext.
class URLRequestContextFactory::MainURLRequestContextGetter
    : public net::URLRequestContextGetter {
 public:
  MainURLRequestContextGetter(
      URLRequestContextFactory* factory,
      content::BrowserContext* browser_context,
      content::ProtocolHandlerMap* protocol_handlers,
      content::URLRequestInterceptorScopedVector request_interceptors)
      : browser_context_(browser_context),
        factory_(factory),
        request_interceptors_(std::move(request_interceptors)) {
    std::swap(protocol_handlers_, *protocol_handlers);
  }

  net::URLRequestContext* GetURLRequestContext() override {
    if (!request_context_) {
      request_context_.reset(factory_->CreateMainRequestContext(
          browser_context_, &protocol_handlers_,
          std::move(request_interceptors_)));
      protocol_handlers_.clear();
    }
    return request_context_.get();
  }

  scoped_refptr<base::SingleThreadTaskRunner> GetNetworkTaskRunner()
      const override {
    return BrowserThread::GetTaskRunnerForThread(content::BrowserThread::IO);
  }

 private:
  ~MainURLRequestContextGetter() override {}

  content::BrowserContext* const browser_context_;
  URLRequestContextFactory* const factory_;
  content::ProtocolHandlerMap protocol_handlers_;
  content::URLRequestInterceptorScopedVector request_interceptors_;
  std::unique_ptr<net::URLRequestContext> request_context_;

  DISALLOW_COPY_AND_ASSIGN(MainURLRequestContextGetter);
};

URLRequestContextFactory::URLRequestContextFactory(
    net::NetworkDelegate* delegate)
    : system_dependencies_initialized_(false),
      main_dependencies_initialized_(false),
      media_dependencies_initialized_(false) {
  network_delegate_.reset(delegate);
}

URLRequestContextFactory::~URLRequestContextFactory() {
}

void URLRequestContextFactory::InitializeOnUIThread() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::UI));
  // Cast http user agent settings must be initialized in UI thread
  // because it registers itself to pref notification observer which is not
  // thread safe.
  http_user_agent_settings_.reset(new AppRuntimeHttpUserAgentSettings());
}

net::URLRequestContextGetter* URLRequestContextFactory::CreateMainGetter(
    content::BrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  if (!main_getter_) {
    main_getter_ =
        new MainURLRequestContextGetter(this, browser_context, protocol_handlers,
                                        std::move(request_interceptors));
  }
  return main_getter_.get();
}

net::URLRequestContextGetter* URLRequestContextFactory::GetMainGetter() {
  CHECK(main_getter_);
  return main_getter_.get();
}

net::URLRequestContextGetter* URLRequestContextFactory::GetSystemGetter() {
  if (!system_getter_) {
    system_getter_ = new URLRequestContextGetter(this, false);
  }
  return system_getter_.get();
}

net::URLRequestContextGetter* URLRequestContextFactory::GetMediaGetter() {
  if (!media_getter_) {
    media_getter_ = new URLRequestContextGetter(this, true);
  }
  return media_getter_.get();
}

void URLRequestContextFactory::InitializeSystemContextDependencies() {
  if (system_dependencies_initialized_)
    return;
  // need for enable AppRuntimeNetworkDelegate to open files
  base::ThreadRestrictions::SetIOAllowed(true);
  if (!network_delegate_)
    network_delegate_.reset(new AppRuntimeNetworkDelegate());

  host_resolver_ = net::HostResolver::CreateDefaultResolver(NULL);
  // host_resolver_->SetDefaultAddressFamily(net::ADDRESS_FAMILY_UNSPECIFIED);

  // TODO(lcwu): http://crbug.com/392352. For performance and security reasons,
  // a persistent (on-disk) HttpServerProperties and ChannelIDService might be
  // desirable in the future.
  channel_id_service_.reset(
      new net::ChannelIDService(new net::DefaultChannelIDStore(NULL)));

  cert_verifier_ = net::CertVerifier::CreateDefault();
  cert_transparency_verifier_.reset(new net::MultiLogCTVerifier);

  std::vector<scoped_refptr<const net::CTLogVerifier>> ct_logs;
  for (const auto& ct_log : certificate_transparency::GetKnownLogs()) {
    scoped_refptr<const net::CTLogVerifier> log_verifier =
        net::CTLogVerifier::Create(
            std::string(ct_log.log_key, ct_log.log_key_length), ct_log.log_name,
            ct_log.log_dns_domain);
    if (!log_verifier) {
      continue;
    }
    ct_logs.push_back(std::move(log_verifier));
  }

  cert_transparency_verifier_->AddLogs(ct_logs);

  ct_policy_enforcer_.reset(new net::DefaultCTPolicyEnforcer());

  ssl_config_service_ = new net::SSLConfigServiceDefaults;

  transport_security_state_.reset(new net::TransportSecurityState());
  http_auth_handler_factory_ =
      net::HttpAuthHandlerFactory::CreateDefault(host_resolver_.get());

  http_server_properties_.reset(new net::HttpServerPropertiesImpl);

  std::unique_ptr<net::ProxyConfigService> proxy_config_service;
  net::ProxyConfig proxy_config;

  proxy_config_service.reset(new net::ProxyConfigServiceFixed(
      net::ProxyConfigWithAnnotation::CreateDirect()));

  proxy_resolution_service_ =
      net::ProxyResolutionService::CreateUsingSystemProxyResolver(
          std::move(proxy_config_service), nullptr);

  system_dependencies_initialized_ = true;
}

void URLRequestContextFactory::InitializeMainContextDependencies(
    net::HttpTransactionFactory* transaction_factory,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  if (main_dependencies_initialized_)
    return;

  main_transaction_factory_.reset(transaction_factory);
  std::unique_ptr<net::URLRequestJobFactoryImpl> job_factory(
      new net::URLRequestJobFactoryImpl());
  // Keep ProtocolHandlers added in sync with
  // CastContentBrowserClient::IsHandledURL().
  bool set_protocol = false;
  for (content::ProtocolHandlerMap::iterator it = protocol_handlers->begin();
       it != protocol_handlers->end(); ++it) {
    set_protocol = job_factory->SetProtocolHandler(
        it->first, std::unique_ptr<net::URLRequestJobFactory::ProtocolHandler>(
                       it->second.release()));
    DCHECK(set_protocol);
  }

  set_protocol = job_factory->SetProtocolHandler(
      url::kFileScheme,
      std::make_unique<net::FileProtocolHandler>(
        base::CreateTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
          base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})));
  DCHECK(set_protocol);

  // Set up interceptors in the reverse order.
  std::unique_ptr<net::URLRequestJobFactory> top_job_factory(
      std::move(job_factory));
  for (content::URLRequestInterceptorScopedVector::reverse_iterator i =
           request_interceptors.rbegin();
       i != request_interceptors.rend(); ++i) {
    top_job_factory.reset(new net::URLRequestInterceptingJobFactory(
        std::move(top_job_factory),
        std::move(*i)));
  }
  request_interceptors.clear();
  main_job_factory_ = std::move(top_job_factory);
  main_dependencies_initialized_ = true;
}

void URLRequestContextFactory::InitializeMediaContextDependencies(
    net::HttpTransactionFactory* transaction_factory) {
  if (media_dependencies_initialized_)
    return;

  media_transaction_factory_.reset(transaction_factory);
  media_dependencies_initialized_ = true;
}

void URLRequestContextFactory::PopulateNetworkSessionParams(
    bool ignore_certificate_errors,
    net::HttpNetworkSession::Params* params) {
  params->ignore_certificate_errors = ignore_certificate_errors;
}

void URLRequestContextFactory::PopulateNetworkSessionContext(
    net::HttpNetworkSession::Context* context) {
  context->host_resolver = host_resolver_.get();
  context->cert_verifier = cert_verifier_.get();
  context->channel_id_service = channel_id_service_.get();
  context->ssl_config_service = ssl_config_service_.get();
  context->transport_security_state = transport_security_state_.get();
  context->http_auth_handler_factory = http_auth_handler_factory_.get();
  context->http_server_properties = http_server_properties_.get();
  context->proxy_resolution_service = proxy_resolution_service_.get();
  context->ct_policy_enforcer = ct_policy_enforcer_.get();
  context->cert_transparency_verifier = cert_transparency_verifier_.get();
  // TODO(lcwu): http://crbug.com/329681. Remove this once spdy is enabled
  // by default at the content level.
  // params->next_protos = net::NextProtosSpdy31();
  // params->use_alternate_protocols = true;
}

net::URLRequestContext* URLRequestContextFactory::CreateSystemRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  InitializeSystemContextDependencies();
  net::HttpNetworkSession::Params network_session_params;
  net::HttpNetworkSession::Context network_session_context;
  PopulateNetworkSessionParams(false, &network_session_params);
  PopulateNetworkSessionContext(&network_session_context);
  system_transaction_factory_.reset(
      new net::HttpNetworkLayer(new net::HttpNetworkSession(network_session_params,
                                                            network_session_context)));

  net::URLRequestContext* system_context = new net::URLRequestContext();
  system_context->set_host_resolver(host_resolver_.get());
  system_context->set_channel_id_service(channel_id_service_.get());
  system_context->set_cert_verifier(cert_verifier_.get());
  system_context->set_cert_transparency_verifier(
      cert_transparency_verifier_.get());
  system_context->set_ct_policy_enforcer(ct_policy_enforcer_.get());
  system_context->set_proxy_resolution_service(proxy_resolution_service_.get());
  system_context->set_ssl_config_service(ssl_config_service_.get());
  system_context->set_transport_security_state(transport_security_state_.get());
  system_context->set_http_auth_handler_factory(
      http_auth_handler_factory_.get());
  system_context->set_http_server_properties(http_server_properties_.get());
  system_context->set_http_transaction_factory(
      system_transaction_factory_.get());
  system_context->set_http_user_agent_settings(http_user_agent_settings_.get());

  cookie_store_ = content::CreateCookieStore(content::CookieStoreConfig());
  system_context->set_cookie_store(cookie_store_.get());
  return system_context;
}

net::URLRequestContext* URLRequestContextFactory::CreateMediaRequestContext() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  DCHECK(main_getter_)
      << "Getting MediaRequestContext before MainRequestContext";
  net::URLRequestContext* main_context = main_getter_->GetURLRequestContext();

  // Set non caching backend.
  net::HttpNetworkSession* main_session =
      main_transaction_factory_->GetSession();
  InitializeMediaContextDependencies(new net::HttpNetworkLayer(main_session));

  net::URLRequestContext* media_context = new net::URLRequestContext();
  media_context->CopyFrom(main_context);
  media_context->set_http_transaction_factory(media_transaction_factory_.get());
  return media_context;
}

net::URLRequestContext* URLRequestContextFactory::CreateMainRequestContext(
    content::BrowserContext* browser_context,
    content::ProtocolHandlerMap* protocol_handlers,
    content::URLRequestInterceptorScopedVector request_interceptors) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));
  InitializeSystemContextDependencies();

  int disk_cache_size = kDefaultDiskCacheSize;
  base::CommandLine* cmd_line = base::CommandLine::ForCurrentProcess();
  if (cmd_line->HasSwitch(kDiskCacheSize))
    base::StringToInt(cmd_line->GetSwitchValueASCII(kDiskCacheSize),
                      &disk_cache_size);

  std::unique_ptr<net::HttpCache::DefaultBackend> main_backend =
      std::make_unique<net::HttpCache::DefaultBackend>(
          net::DISK_CACHE, net::CACHE_BACKEND_SIMPLE,
          browser_context->GetPath().Append(kCacheStoreFile), disk_cache_size);

  bool ignore_certificate_errors = false;
  if (cmd_line->HasSwitch(switches::kIgnoreCertificateErrors)) {
    ignore_certificate_errors = true;
  }
  net::HttpNetworkSession::Params network_session_params;
  net::HttpNetworkSession::Context network_session_context;
  PopulateNetworkSessionParams(ignore_certificate_errors,
                               &network_session_params);
  PopulateNetworkSessionContext(&network_session_context);

  network_session_ =
      std::make_unique<net::HttpNetworkSession>(network_session_params,
                                                network_session_context);

  InitializeMainContextDependencies(
      new net::HttpCache(network_session_.get(), std::move(main_backend),
#if defined(ENABLE_EMMC_OPTIMIZATIONS)
                         kCacheMinContentLength, true,
#else
                         0, false,
#endif
                         false),
      protocol_handlers, std::move(request_interceptors));

  content::CookieStoreConfig cookie_config(
      browser_context->GetPath().Append(kCookieStoreFile), false, true, NULL);
  cookie_config.background_task_runner =
      scoped_refptr<base::SequencedTaskRunner>();
  cookie_config.crypto_delegate = cookie_config::GetCookieCryptoDelegate();

  net::URLRequestContext* main_context = new net::URLRequestContext();
  main_context->set_host_resolver(host_resolver_.get());
  main_context->set_channel_id_service(channel_id_service_.get());
  main_context->set_cert_verifier(cert_verifier_.get());
  main_context->set_cert_transparency_verifier(
      cert_transparency_verifier_.get());
  main_context->set_ct_policy_enforcer(ct_policy_enforcer_.get());
  main_context->set_proxy_resolution_service(proxy_resolution_service_.get());
  main_context->set_ssl_config_service(ssl_config_service_.get());
  main_context->set_transport_security_state(transport_security_state_.get());
  main_context->set_http_auth_handler_factory(http_auth_handler_factory_.get());
  main_context->set_http_server_properties(http_server_properties_.get());
  cookie_store_ = content::CreateCookieStore(cookie_config);
  main_context->set_cookie_store(cookie_store_.get());
  main_context->set_http_user_agent_settings(http_user_agent_settings_.get());
  main_context->set_http_transaction_factory(main_transaction_factory_.get());
  main_context->set_network_delegate(network_delegate_.get());
  main_context->set_job_factory(main_job_factory_.get());

  const base::CommandLine* command_line =
      base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(switches::kEnableLocalResourceCodeCache)) {
    int max_size = 5242880;
    if (command_line->HasSwitch(switches::kLocalResourceCodeCacheSize))
      base::StringToInt(command_line->GetSwitchValueASCII(
                            switches::kLocalResourceCodeCacheSize),
                        &max_size);
    code_cache_.reset(
        new net::CodeCacheImpl(browser_context->GetPath(), max_size));
  } else {
    code_cache_.reset(new net::DummyCodeCache(browser_context->GetPath(), 0));
    // Delete code cache directory in storage if it exists
    code_cache_->ClearData();
  }

  main_context->set_code_cache(code_cache_.get());

  return main_context;
}

void URLRequestContextFactory::SetProxyServer(
    const std::string& ip,
    const std::string& port,
    const std::string& name,
    const std::string& password,
    const std::string& proxy_bypass_list) {
  if (!proxy_resolution_service_) {
    std::unique_ptr<net::ProxyConfigService> proxy_config_service;
    proxy_config_service.reset(new net::ProxyConfigServiceFixed(
        net::ProxyConfigWithAnnotation::CreateDirect()));

    proxy_resolution_service_ =
        net::ProxyResolutionService::CreateUsingSystemProxyResolver(
            std::move(proxy_config_service), nullptr);
  } else if (!ip.empty() && !port.empty()) {
    std::string proxy_string = ip + ":" + port;
    net::ProxyServer proxy_for_http =
        net::ProxyServer::FromURI(proxy_string, net::ProxyServer::SCHEME_HTTP);
    if (!proxy_for_http.is_valid())
      return;

    proxy_for_http.SetAuth(net::AuthCredentials(
        base::UTF8ToUTF16(name.c_str()), base::UTF8ToUTF16(password.c_str())));

    net::ProxyConfig proxy_config;
    proxy_config.proxy_rules().type =
        net::ProxyConfig::ProxyRules::Type::PROXY_LIST;
    proxy_config.proxy_rules().single_proxies.SetSingleProxyServer(
        proxy_for_http);
    if (!proxy_bypass_list.empty()) {
      // Note that this uses "suffix" matching. So a bypass of "google.com"
      // is understood to mean a bypass of "*google.com".
      proxy_config.proxy_rules()
          .bypass_rules.ParseFromStringUsingSuffixMatching(proxy_bypass_list);
    }
    proxy_config.set_auto_detect(false);

    std::unique_ptr<net::ProxyConfigService> proxy_config_service =
        std::make_unique<net::ProxyConfigServiceFixed>(
            net::ProxyConfigWithAnnotation(
                proxy_config,
                proxy_resolution_service_->config()->traffic_annotation()));
    proxy_resolution_service_->ResetConfigService(
        std::move(proxy_config_service));
  }
}

}  // namespace app_runtime
