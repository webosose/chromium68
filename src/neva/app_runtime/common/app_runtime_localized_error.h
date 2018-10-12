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

#ifndef NEVA_APP_RUNTIME_COMMON_APP_RUNTIME_LOCALIZED_ERROR_H_
#define NEVA_APP_RUNTIME_COMMON_APP_RUNTIME_LOCALIZED_ERROR_H_

#include <string>

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace blink {
struct WebURLError;
}

namespace error_page {

struct ErrorPageParams;

class AppRuntimeLocalizedError {
 public:
  // Fills |error_strings| with values to be used to build an error page used
  // on HTTP errors, like 404 or connection reset.
  static void GetStrings(int error_code,
                         const std::string& error_domain,
                         const GURL& failed_url,
                         bool is_post,
                         bool show_stale_load_button,
                         const std::string& locale,
                         const std::string& accept_languages,
                         std::unique_ptr<ErrorPageParams> params,
                         base::DictionaryValue* strings);

  // Returns a description of the encountered error.
  static base::string16 GetErrorDetails(const std::string& error_domain,
                                        int error_code,
                                        bool is_post);

  // Returns true if an error page exists for the specified parameters.
  static bool HasStrings(const std::string& error_domain, int error_code);

  static const char kHttpErrorDomain[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(AppRuntimeLocalizedError);
};

}  // namespace error_page

#endif  // NEVA_APP_RUNTIME_COMMON_APP_RUNTIME_LOCALIZED_ERROR_H_
