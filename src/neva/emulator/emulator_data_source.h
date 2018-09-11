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

#ifndef EMULATOR_EMULATOR_DATA_SOURCE_H_
#define EMULATOR_EMULATOR_DATA_SOURCE_H_

#include <map>
#include <memory>
#include <mutex>
#include <string>

#include "base/threading/thread.h"
#include "emulator/emulator_export.h"
#include "net/url_request/url_fetcher_delegate.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
class TaskRunner;
}  // namespace base

namespace net {
class URLFetcher;
class URLRequestContext;
}  // namespace net

namespace emulator {

// Delegate interface for EmulatorDataSource consumers

class EmulatorDataDelegate {
 public:
  virtual void DataUpdated(const std::string& url, const std::string& data) = 0;
};

struct RequestArgumentDescription {
  const char *name;
  const std::string *value;
};

struct ResponseArgumentDescription {
  const char *name;
  std::string *value;
};

using RequestArgs = std::vector<RequestArgumentDescription>;
using ResponseArgs = std::vector<ResponseArgumentDescription>;

// This is a singleton which manages a single thread for HTTP requests for any
// stub data.

class EMULATOR_EXPORT EmulatorDataSource : public base::Thread,
                                           public net::URLFetcherDelegate {
 public:
  static EmulatorDataSource* GetInstance();
  ~EmulatorDataSource() override;

  // Set expectation on the Mock server. This call is asynchronous.
  // The implementation of the function is executed
  // on the EmulatorDataSource thread.
  static void SetExpectationAsync(const std::string& url,
                                  const std::string& arg);

  void AddURLForPolling(const std::string& url,
                        EmulatorDataDelegate* delegate,
                        const scoped_refptr<base::TaskRunner>& taskRunner);
  std::string GetCachedValueForURL(const std::string& url);
  static std::string PrepareRequestParams(RequestArgs &args);
  static std::string PrepareRequestParams(base::DictionaryValue& request);
  static bool GetResponseParams(ResponseArgs &args,
                                const std::string& response);

  // from net::URLFetcherDelegate
  void OnURLFetchComplete(const net::URLFetcher* source) override;
  void OnURLFetchDownloadProgress(const net::URLFetcher* source,
                                  int64_t current,
                                  int64_t total,
                                  int64_t current_network_bytes) override;
  void OnURLFetchUploadProgress(const net::URLFetcher* source,
                                int64_t current,
                                int64_t total) override;

 private:
  EmulatorDataSource();
  void PeriodicPoll();
  void SetExpectation(const std::string& url, const std::string& arg);
  void FetchURLOnce(const std::string& fetch_url);

  // from base::Thread()
  void Init() override;

  static std::unique_ptr<EmulatorDataSource> instance_;
  static const int kPollingIntervalMs = 100;
  static std::string kEmulatorBaseURL;
  static GURL kExpectationURL;

  struct URLData;
  std::map<std::string, URLData> urls_;
  std::mutex mutex_;
  std::unique_ptr<net::URLRequestContext> url_request_context_;
  std::set<std::unique_ptr<const net::URLFetcher>> url_fetchers_;
  std::set<std::unique_ptr<const net::URLFetcher>> expectation_fetchers_;
};
}  // namespace emulator

#endif  // EMULATOR_EMULATOR_DATA_SOURCE_H_
