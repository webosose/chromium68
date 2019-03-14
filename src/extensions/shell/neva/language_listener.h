// Copyright 2019 LG Electronics, Inc.
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

#ifndef EXTENSIONS_SHELL_NEVA_LANGUAGE_LISTENER_H_
#define EXTENSIONS_SHELL_NEVA_LANGUAGE_LISTENER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"
#include "pal/public/interfaces/system_locale_interface.h"

namespace pal {
class SystemLocaleInterface;
}

namespace neva {

class LanguageListener : public content::WebContentsUserData<LanguageListener>,
                         public content::WebContentsObserver {
 public:
  ~LanguageListener() override;

 private:
  friend class content::WebContentsUserData<LanguageListener>;

  explicit LanguageListener(content::WebContents* web_contents);

  void LocaleInfoUpdated(const std::string& locale_info_json);

  pal::SystemLocaleInterface* interface_;

  pal::SystemLocaleInterface::SystemLocaleChangedCallback
      localeInfoChangedCallback_;
  std::unique_ptr<pal::SystemLocaleInterface::SystemLocaleChangedSubscription>
      localeInfoChangedSubscription_;
};

}  // namespace neva

#endif  // EXTENSIONS_SHELL_NEVA_LANGUAGE_LISTENER_H_
