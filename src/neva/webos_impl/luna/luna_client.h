// Copyright 2018 LG Electronics, Inc.
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

#ifndef NEVA_WEBOS_IMPL_LUNA_LUNA_CLIENT_H
#define NEVA_WEBOS_IMPL_LUNA_LUNA_CLIENT_H

#include <string>

#include "base/macros.h"

struct LSHandle;

namespace lunabus {

class Handler {
 public:
  Handler() = default;
  virtual ~Handler();

  void SetToken(unsigned int token);
  unsigned int GetToken() const;

  virtual void Handle(const char* payload) = 0;

 private:
  unsigned int token_ = 0;
};

class LunaClient {
 public:
  explicit LunaClient(const char* id);
  ~LunaClient();

  bool Initialized() const;
  unsigned long Call(const char* uri,
                     const char* payload,
                     Handler* handler,
                     const bool subscription = false);
  unsigned long Signal(const char* payload, Handler* handler);
  void Cancel(unsigned int token);

 private:
  bool Initialize(const char* id);

  LSHandle* handle_ = nullptr;
  std::string id_;
  bool initialized_ = false;
  DISALLOW_COPY_AND_ASSIGN(LunaClient);
};

}  // namespace lunabus

#endif  // NEVA_WEBOS_IMPL_LUNA_LUNA_CLIENT_H
