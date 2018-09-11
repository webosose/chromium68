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

#ifndef NEVA_PAL_PUBLIC_PAL_H_
#define NEVA_PAL_PUBLIC_PAL_H_

#include "pal/ipc/pal_export.h"

namespace pal {

#include "pal/public/pal_classes_gen.h"
// Singleton class. Provides public access to all pal's interfaces.

class PAL_EXPORT Pal {
 public:
  Pal();
  virtual ~Pal();

  // Must be implemented for each platform.
  // Return PAL's implementation for current platform.
  static Pal* GetPlatformInstance();

  #include "pal/public/pal_gen.h"

  // Common function for PAL. Closes all components that were created
  // by interfaces in current implementation and frees resources.
  virtual void PlatformShutdown();
};

}  // namespace pal

#endif  // NEVA_PAL_PUBLIC_PAL_H_
