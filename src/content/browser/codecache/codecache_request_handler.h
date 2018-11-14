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

#ifndef CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_
#define CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_

#include "base/compiler_specific.h"
#include "base/supports_user_data.h"
#include "content/common/content_export.h"
#include "content/public/common/resource_type.h"
#include "url/gurl.h"

namespace net {
class NetworkDelegate;
class URLRequest;
class URLRequestJob;
}  // namespace net

namespace content {
class CodeCacheURLRequestJob;

// An instance is created for supporting v8 code cache for file:// scheme
class CONTENT_EXPORT CodeCacheRequestHandler
    : public base::SupportsUserData::Data {
 public:
  CodeCacheRequestHandler();

  CodeCacheURLRequestJob* MaybeLoadResource(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate);

 private:
  bool request_job_delivered_;

  DISALLOW_COPY_AND_ASSIGN(CodeCacheRequestHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CODECACHE_CODECACHE_REQUEST_HANDLER_H_
