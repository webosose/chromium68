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

#ifndef NEVA_LOGGING_H_
#define NEVA_LOGGING_H_

#include "base/logging.h"

#if defined(DCHECK_ALWAYS_ON)
#define NEVA_DCHECK_ALWAYS_ON 1
#endif

#if defined(NDEBUG) && !defined(NEVA_DCHECK_ALWAYS_ON)
// 'Unforced' release configuration
#define NEVA_DCHECK_IS_ON()   0
#else
// 'Forced' release or debug configuration
#define NEVA_DCHECK_IS_ON()   1
#endif

#define NEVA_DCHECK(condition)                                                 \
    LAZY_STREAM(LOG_STREAM(FATAL), NEVA_DCHECK_IS_ON() ? !(condition) : false) \
    << "(LG) Check failed: " #condition ". "

#endif  // NEVA_LOGGING_H_
