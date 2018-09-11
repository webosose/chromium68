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

#ifndef NEVA_PAL_PUBLIC_COMMON_CALLBACK_HANDLER_H_
#define NEVA_PAL_PUBLIC_COMMON_CALLBACK_HANDLER_H_

#include "pal/common/pal_export.h"

#include <set>

namespace pal {

template <typename DelegateType>
class PAL_EXPORT CallbackHandler {
 public:
  virtual ~CallbackHandler() {}

  void AddDelegate(DelegateType* cb);
  void RemoveDelegate(DelegateType* cb);

 protected:
  std::set<DelegateType*> delegates_;
};

template <class DelegateType>
void CallbackHandler<DelegateType>::AddDelegate(DelegateType* delegate) {
  delegates_.insert(delegate);
}

template <class DelegateType>
void CallbackHandler<DelegateType>::RemoveDelegate(DelegateType* delegate) {
  delegates_.erase(delegate);
}

#define FOR_EACH_DELEGATE(delegates, func)                       \
  for (auto it = delegates.begin(); it != delegates.end(); ++it) \
    (*it)->func;

}  // namespace pal

#endif  // NEVA_PAL_PUBLIC_COMMON_CALLBACK_HANDLER_H_
