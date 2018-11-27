// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "webos/webview_profile.h"

#include "base/time/time.h"
#include "neva/app_runtime/webview_profile.h"

namespace webos {

WebViewProfile::WebViewProfile(const std::string& storage_name)
    : profile_(new app_runtime::WebViewProfile(storage_name)),
      is_default_(false) {}

WebViewProfile::WebViewProfile(app_runtime::WebViewProfile* default_profile)
    : profile_(default_profile), is_default_(true) {}

WebViewProfile* WebViewProfile::GetDefaultProfile() {
  static WebViewProfile* profile =
      new WebViewProfile(app_runtime::WebViewProfile::GetDefaultProfile());
  return profile;
}

WebViewProfile::~WebViewProfile() {
  if (!is_default_)
    delete profile_;
}

void WebViewProfile::AppendExtraWebSocketHeader(const std::string& key,
                                                const std::string& value) {
  profile_->AppendExtraWebSocketHeader(key, value);
}

void WebViewProfile::FlushCookieStore() {
  profile_->FlushCookieStore();
}

void WebViewProfile::RemoveBrowsingData(int remove_browsing_data_mask) {
  profile_->RemoveBrowsingData(remove_browsing_data_mask);
}

}  // namespace webos
