// Copyright (c) 2018 LG Electronics, Inc.
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

#include "emulator_data_source.h"

#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/values.h"
#include "emulator_urls.h"
#include "net/http/http_status_code.h"
#if defined(OS_LINUX)
#include "net/proxy_resolution/proxy_config.h"
#include "net/proxy_resolution/proxy_config_service_fixed.h"
#endif
#include "net/url_request/url_fetcher.h"
#include "net/url_request/url_request_context.h"
#include "net/url_request/url_request_context_builder.h"
#include "net/url_request/url_request_context_getter.h"

namespace {

const char kUploadContentType[] = "text/plain; charset=utf-8";

// Builds a URLRequestContext assuming there's only a single loop.
std::unique_ptr<net::URLRequestContext> BuildURLRequestContext() {
  net::URLRequestContextBuilder builder;
#if defined(OS_LINUX)
  // On Linux, use a fixed ProxyConfigService, since the default one
  // depends on glib.
  //
  // TODO(akalin): Remove this once http://crbug.com/146421 is fixed.
  builder.set_proxy_config_service(
      std::make_unique<net::ProxyConfigServiceFixed>(
          net::ProxyConfigWithAnnotation()));
#endif
  std::unique_ptr<net::URLRequestContext> context(builder.Build());
  return context;
}

}  // anonymous namespace

namespace emulator {

struct EmulatorDataSource::URLData {
  URLData() {}
  ~URLData() {}
  EmulatorDataDelegate* delegate;
  std::string cached_data;
  scoped_refptr<base::TaskRunner> task_runner;
  std::string url;
};

std::unique_ptr<EmulatorDataSource> EmulatorDataSource::instance_;
std::string EmulatorDataSource::kEmulatorBaseURL;
GURL EmulatorDataSource::kExpectationURL;

EmulatorDataSource* EmulatorDataSource::GetInstance() {
  char *neva_emulator_server_address = ::getenv("NEVA_EMULATOR_SERVER_ADDRESS");

  kEmulatorBaseURL = "http://" +
                     std::string(
                        (neva_emulator_server_address != nullptr) ?
                        neva_emulator_server_address:kEmulatorDefaultHost
                     ) + ":" + std::to_string(kEmulatorDefaultPort) + "/";

  kExpectationURL = GURL((kEmulatorBaseURL + kEmulatorExpectationPath).c_str());

  if (instance_ == nullptr)
    instance_.reset(new EmulatorDataSource());

  return instance_.get();
}

EmulatorDataSource::EmulatorDataSource()
    : base::Thread("Emulator_DataFetcherThread") {
  Options options;
  options.message_loop_type = base::MessageLoop::TYPE_IO;
  StartWithOptions(options);
}

void EmulatorDataSource::Init() {
  url_request_context_ = BuildURLRequestContext();

  // Start polling
  PeriodicPoll();
}

EmulatorDataSource::~EmulatorDataSource() {
  Stop();
}

void EmulatorDataSource::AddURLForPolling(
    const std::string& url,
    EmulatorDataDelegate* delegate,
    const scoped_refptr<base::TaskRunner>& taskRunner) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string fetch_url = kEmulatorBaseURL + url;
  URLData& url_data = urls_[fetch_url];
  url_data.delegate = delegate;
  url_data.task_runner = taskRunner;
  url_data.url = url;
}

std::string EmulatorDataSource::GetCachedValueForURL(const std::string& url) {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string fetch_url = kEmulatorBaseURL + url;
  return urls_[fetch_url].cached_data;
}

void EmulatorDataSource::FetchURLOnce(const std::string& fetch_url) {
  std::unique_ptr<net::URLFetcher> url_fetcher;
  url_fetcher = net::URLFetcher::Create(GURL(fetch_url.c_str()),
                                        net::URLFetcher::GET,
                                        this);

  url_fetcher->SetRequestContext(
      // Since there's only a single thread, there's no need to worry
      // about when the URLRequestContext gets created.
      // The URLFetcher will take a reference on the object, and hence
      // implicitly take ownership.
      new net::TrivialURLRequestContextGetter(url_request_context_.get(),
                                              task_runner()));

  url_fetcher->Start();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    url_fetchers_.emplace(std::move(url_fetcher));
  }
}

void EmulatorDataSource::PeriodicPoll() {
  std::vector<std::string> urls;

  // Extracting URL values from the URL entries added for polling
  {
    std::lock_guard<std::mutex> lock(mutex_);
    for (auto it = urls_.begin(); it != urls_.end(); ++it)
      urls.push_back(it->first);
  }

  // Traversing the URL values just extracted, trying to fetch related data
  for (auto it = urls.begin(); it != urls.end(); ++it)
    FetchURLOnce(*it);

  // NOTE: the recursive call for polling below provides (most probably) one and
  // only correct alternative to polling via |base::Timer| instance while using
  // the Chromium |net| API set (otherwise, |base::Timer| and |net| library
  // become incompatible with each other)
  task_runner()->PostDelayedTask(FROM_HERE,
                                 base::Bind(&EmulatorDataSource::PeriodicPoll,
                                            base::Unretained(this)),
                                 base::TimeDelta::FromMilliseconds(
                                     kPollingIntervalMs));
}

void EmulatorDataSource::SetExpectation(const std::string& url,
                                        const std::string& arg) {
  std::unique_ptr<net::URLFetcher> url_fetcher;
  std::string request =
      R"JSON({"httpRequest":{"method":"","path":"/)JSON" + url +
      R"JSON(","queryStringParameters":[],"body":"","headers":[],"cookies":[]},
      )JSON";
  std::string response =
      R"JSON("httpResponse":{"statusCode":200,"body":")JSON" + arg +
      R"JSON(","cookies":[],"headers":[{"name":"Content-Type","values":
      ["text/plain;charset=utf-8"]},{"name":"Cache-Control","values":
      ["no-cache,no-store"]}],"delay":{"timeUnit":"MICROSECONDS","value":0}},
      )JSON";
  std::string options =
      R"JSON("times":{"remainingTimes":1,"unlimited":false}})JSON";

  url_fetcher = net::URLFetcher::Create(kExpectationURL,
                                        net::URLFetcher::PUT,
                                        this);

  url_fetcher->SetRequestContext(
      // Since there's only a single thread, there's no need to worry
      // about when the URLRequestContext gets created.
      // The URLFetcher will take a reference on the object, and hence
      // implicitly take ownership.
      new net::TrivialURLRequestContextGetter(url_request_context_.get(),
                                              task_runner()));

  url_fetcher->SetUploadData(kUploadContentType, request + response + options);

  url_fetcher->Start();

  {
    std::lock_guard<std::mutex> lock(mutex_);
    expectation_fetchers_.emplace(std::move(url_fetcher));
  }
}

void EmulatorDataSource::SetExpectationAsync(const std::string& url,
                                             const std::string& arg) {
  EmulatorDataSource* instance = GetInstance();

  instance->task_runner()->PostTask(FROM_HERE,
      base::Bind(&EmulatorDataSource::SetExpectation,
                 base::Unretained(instance),
                 url,
                 arg));
}

std::string EmulatorDataSource::PrepareRequestParams(RequestArgs &args) {
  base::DictionaryValue request;
  for (auto arg: args) {
    request.SetString((arg.name), *(arg.value));
  }
  return PrepareRequestParams(request);
}

std::string EmulatorDataSource::PrepareRequestParams(
    base::DictionaryValue& request) {
  std::string params;
  base::JSONWriter::Write(request, &params);
  std::string esc_params;
  base::EscapeJSONString(params, false, &esc_params);
  return esc_params;
}

bool EmulatorDataSource::GetResponseParams(ResponseArgs &args,
    const std::string& response) {
  std::string error_msg;
  std::unique_ptr<base::Value> response_value =
      base::JSONReader::ReadAndReturnError(response, base::JSON_PARSE_RFC,
                                           nullptr, &error_msg);
  if (response_value == nullptr) {
    LOG(ERROR) << __func__ << "() : JSONReader failed : " << error_msg;
    return false;
  }

  if (!response_value->is_dict()) {
    LOG(ERROR) << __func__ << "() : Unexpected response type "
            << response_value->type();
    return false;
  }

  const base::DictionaryValue* response_object =
      static_cast<base::DictionaryValue*>(response_value.get());

  for (auto arg: args) {
    if (!response_object->GetString(arg.name, arg.value)) {
          LOG(ERROR) << __func__ << "() : Absent argument '"
            << arg.name << "'";
          return false;
    }
  }
  return true;
}

void EmulatorDataSource::OnURLFetchComplete(const net::URLFetcher* source) {
  std::unique_ptr<const net::URLFetcher> fetcher(source);
  {
    std::lock_guard<std::mutex> lock(mutex_);

    auto expectation_result = expectation_fetchers_.find(fetcher);
    auto url_result = url_fetchers_.find(fetcher);
    fetcher.release();

    if (expectation_result != expectation_fetchers_.end()) {
      const net::URLRequestStatus status = source->GetStatus();
      if (status.status() != net::URLRequestStatus::SUCCESS) {
          LOG(ERROR) << __func__ << "(): Request failed with error code: "
                     << net::ErrorToString(status.error());
      }
      expectation_fetchers_.erase(expectation_result);
      return;
    }

    if (url_result != url_fetchers_.end()) {
      std::string fetch_url = source->GetURL().spec();
      URLData& url_data = urls_[fetch_url];

      const net::URLRequestStatus status = source->GetStatus();
      if (status.status() != net::URLRequestStatus::SUCCESS) {
        LOG(ERROR) << __func__ << "(): Request failed with error code: "
                   << net::ErrorToString(status.error());
      } else {
        int response_code = source->GetResponseCode();
        // Is this a 200 ("OK") response code?
        if (response_code == net::HTTP_OK) {
          std::string data;
          // Populating value by the response received
          if (!source->GetResponseAsString(&data))
            data.clear();

          url_data.cached_data = data;
          url_data.task_runner->PostTask(FROM_HERE,
              base::Bind(&EmulatorDataDelegate::DataUpdated,
              base::Unretained(url_data.delegate),
              url_data.url, data));
        }
      }
      url_fetchers_.erase(url_result);
      return;
    }
  }

  LOG(ERROR) << __func__ << "(): Wrong URLFetcher";
  NOTREACHED();
}

void EmulatorDataSource::OnURLFetchDownloadProgress(
    const net::URLFetcher* source,
    int64_t current,
    int64_t total,
    int64_t current_network_bytes) {
  NOTIMPLEMENTED();
}

void EmulatorDataSource::OnURLFetchUploadProgress(const net::URLFetcher* source,
                                                  int64_t current,
                                                  int64_t total) {
  NOTREACHED();
}

}  // namespace emulator
