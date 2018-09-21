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

#ifndef BASE_LOGGING_PMLOG_H_
#define BASE_LOGGING_PMLOG_H_

#include "base/base_export.h"
#include "base/logging.h"

#if !defined(USE_PMLOG)
#define PMLOG(Context, level, msgid, ...) (void)0
#define PMLOG_DEBUG(Context, ...) (void)0
#define PMLOG_INFO(Context, msgid, ...) (void)0
#define PMLOG_ENABLED(Context) false
#define PMLOG_DEBUG_ENABLED(Context) false
#define PMLOG_INFO_ENABLED(Context) false
#else
#define PMLOG(Context, level, msgid, ...) \
  logging::Context##PmLog(logging::LOG_##level, msgid, __VA_ARGS__)
#define PMLOG_DEBUG(Context, ...) PMLOG(Context, VERBOSE, "", __VA_ARGS__)
#define PMLOG_INFO(Context, msgid, ...) PMLOG(Context, INFO, msgid, __VA_ARGS__)
#define PMLOG_ENABLED(Context, level) \
  logging::Context##PmLogEnabled(logging::LOG_##level)
#define PMLOG_DEBUG_ENABLED(Context) PMLOG_ENABLED(Context, VERBOSE)
#define PMLOG_INFO_ENABLED(Context) PMLOG_ENABLED(Context, INFO)
#endif  // defined(USE_PMLOG)

namespace logging {

#if !defined(USE_PMLOG)
#define DECLARE_PMLOG_HEADER(Context)
#else
#define DECLARE_PMLOG_HEADER(Context)                           \
  BASE_EXPORT void Context##PmLog(int level, const char* msgid, \
                                  const char* format, ...);     \
  BASE_EXPORT bool Context##PmLogEnabled(int level);
#endif  // defined(USE_PMLOG)

DECLARE_PMLOG_HEADER(Raw)

}  // namespace logging

#endif  // BASE_LOGGING_PMLOG_H_
