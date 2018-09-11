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

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_MAIN_EXTRA_PARTS_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_MAIN_EXTRA_PARTS_H_

// Interface class for Parts owned by AppRuntimeBrowserMainParts.
// The default implementation for all methods is empty.

// Most of these map to content::BrowserMainParts methods. This interface is
// separate to allow stages to be further subdivided for AppRuntime specific
// initialization stages (e.g. browser process init, profile init).

// While AppRuntimeBrowserMainParts are platform-specific,
// AppRuntimeBrowserMainExtraParts are used to do further initialization for
// various
// AppRuntime toolkits (e.g., AURA, etc.;)
namespace app_runtime {

class AppRuntimeBrowserMainExtraParts {
 public:
  virtual ~AppRuntimeBrowserMainExtraParts() {}

  virtual void PreMainMessageLoopRun() {}
};

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_BROWSER_MAIN_EXTRA_PARTS_H_
