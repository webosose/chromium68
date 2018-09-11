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

#include "media_info_loader.h"

#include "base/callback_helpers.h"
#include "third_party/blink/public/platform/web_url_error.h"
#include "third_party/blink/public/platform/web_url_loader.h"
#include "third_party/blink/public/platform/web_url_request.h"
#include "third_party/blink/public/platform/web_url_response.h"
#include "third_party/blink/public/web/web_associated_url_loader.h"
#include "third_party/blink/public/web/web_local_frame.h"

namespace media {

static const int kHttpOK = 200;
static const int kHttpPartialContentOK = 206;

MediaInfoLoader::MediaInfoLoader(const GURL& url, const ReadyCB& ready_cb)
    : url_(url), ready_cb_(ready_cb) {}

MediaInfoLoader::~MediaInfoLoader() {}

void MediaInfoLoader::Start(blink::WebLocalFrame* frame) {
  blink::WebURLRequest request(url_);
  request.SetRequestContext(blink::WebURLRequest::kRequestContextVideo);
  frame->SetReferrerForRequest(request, blink::WebURL());

  // To avoid downloading the data use two byte range
  request.AddHTTPHeaderField("Range", "bytes=0-1");

  blink::WebAssociatedURLLoaderOptions options;
  options.expose_all_response_headers = true;
  options.preflight_policy =
      network::mojom::CORSPreflightPolicy::kPreventPreflight;

  std::unique_ptr<blink::WebAssociatedURLLoader> loader(
      frame->CreateAssociatedURLLoader(options));
  loader->LoadAsynchronously(request, this);
  active_loader_ = std::move(loader);
}

/////////////////////////////////////////////////////////////////////////////
// blink::WebURLLoaderClient implementation.
bool MediaInfoLoader::WillFollowRedirect(
    const blink::WebURL& new_url,
    const blink::WebURLResponse& redirectResponse) {
  if (ready_cb_.is_null()) {
    return false;
  }

  url_ = new_url;
  return true;
}

void MediaInfoLoader::DidReceiveResponse(
    const blink::WebURLResponse& response) {
  if (!url_.SchemeIs(url::kHttpScheme) && !url_.SchemeIs(url::kHttpsScheme)) {
    DidBecomeReady(true);
    return;
  }
  if (response.HttpStatusCode() == kHttpOK ||
      response.HttpStatusCode() == kHttpPartialContentOK) {
    DidBecomeReady(true);
    return;
  }
  DidBecomeReady(false);
}

void MediaInfoLoader::DidFinishLoading() {
  DidBecomeReady(true);
}

void MediaInfoLoader::DidFail(const blink::WebURLError& error) {
  DidBecomeReady(false);
}

void MediaInfoLoader::DidBecomeReady(bool ok) {
  active_loader_.reset();
  if (!ready_cb_.is_null())
    base::ResetAndReturn(&ready_cb_).Run(ok, url_);
}

}  // namespace media
