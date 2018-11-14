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

#include "content/browser/codecache/codecache_interceptor.h"

#include "base/command_line.h"
#include "content/browser/codecache/codecache_request_handler.h"
#include "content/browser/codecache/codecache_url_request_job.h"
#include "content/public/common/content_switches.h"
#include "net/url_request/url_request.h"

static int kHandlerKey;  // Value is not used.

namespace content {

void CodeCacheInterceptor::SetExtraRequestInfo(net::URLRequest* request,
                                               ResourceType resource_type) {
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableLocalResourceCodeCache) &&
      request->url().SchemeIsFile() && resource_type == RESOURCE_TYPE_SCRIPT) {
    // Create a handler for this request and associate it with the request.
    // request takes ownership
    request->SetUserData(&kHandlerKey,
                         std::make_unique<CodeCacheRequestHandler>());
  }
}

net::URLRequestJob* CodeCacheInterceptor::MaybeInterceptRequest(
    net::URLRequest* request,
    net::NetworkDelegate* network_delegate) const {
  CodeCacheRequestHandler* handler =
      static_cast<CodeCacheRequestHandler*>(request->GetUserData(&kHandlerKey));
  if (!handler)
    return nullptr;
  return handler->MaybeLoadResource(request, network_delegate);
}

}  // namespace content
