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

#ifndef CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_
#define CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_

#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "net/url_request/url_request_interceptor.h"

class GURL;

namespace net {
class URLRequest;
}

namespace content {
class CodeCacheRequestHandler;

// An interceptor to hijack requests and potentially service them out of
// the codecache.
class CONTENT_EXPORT CodeCacheInterceptor : public net::URLRequestInterceptor {
 public:
  // Must be called to make a request eligible for retrieval from an codecache.
  static void SetExtraRequestInfo(net::URLRequest* request,
                                  ResourceType resource_type);

  CodeCacheInterceptor() {}

 protected:
  // Override from net::URLRequestInterceptor:
  net::URLRequestJob* MaybeInterceptRequest(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) const override;

 private:
  DISALLOW_COPY_AND_ASSIGN(CodeCacheInterceptor);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CODECACHE_CODECACHE_INTERCEPTOR_H_
