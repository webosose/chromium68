// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef CONTENT_PUBLIC_BROWSER_LUNA_SERVICE_DELEGATE_H_
#define CONTENT_PUBLIC_BROWSER_LUNA_SERVICE_DELEGATE_H_

#include <lunaservice.h>

#include "content/common/content_export.h"

namespace content {

class LunaServiceDelegate;

// Setter and getter for the luna service delegate for WebOS. The delegate
// should be set before creating BrowserAccessibilityManager.
CONTENT_EXPORT void SetLunaServiceDelegate(LunaServiceDelegate* delegate);
LunaServiceDelegate* GetLunaServiceDelegate();

class CONTENT_EXPORT LunaServiceDelegate {
 public:
  virtual ~LunaServiceDelegate() {}

  virtual LSHandle* GetHandle() = 0;
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_LUNA_SERVICE_DELEGATE_H_
