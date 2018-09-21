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

#include "base/logging_pmlog.h"

#include <PmLogLib.h>
#include <glib.h>

namespace logging {

#define DECLARE_PMLOG_FOR_CONTEXT_IMPL(Context, string)                        \
  static PmLogContext Context##PmLogContext() {                                \
    static PmLogContext pmlog_context = 0;                                     \
                                                                               \
    if (!pmlog_context)                                                        \
      PmLogGetContext(string, &pmlog_context);                                 \
                                                                               \
    return pmlog_context;                                                      \
  }                                                                            \
  bool Context##PmLogEnabled(int level) {                                      \
    return PmLogIsEnabled(Context##PmLogContext(), level);                     \
  }                                                                            \
  void Context##PmLog(int level, const char* msgid, const char* format, ...) { \
    char buffer[4096] = {0};                                                   \
                                                                               \
    va_list args;                                                              \
    va_start(args, format);                                                    \
    vsnprintf(buffer, sizeof(buffer), format, args);                           \
    va_end(args);                                                              \
                                                                               \
    char* escaped_string = g_strescape(buffer, NULL);                          \
    if (level >= LOG_INFO) {                                                   \
      PmLogInfo(Context##PmLogContext(), msgid, 1,                             \
                PMLOGKS("INFO", escaped_string), "");                          \
    } else {                                                                   \
      PmLogDebug(Context##PmLogContext(), "%s", escaped_string);               \
    }                                                                          \
    g_free(escaped_string);                                                    \
  }

DECLARE_PMLOG_FOR_CONTEXT_IMPL(Raw, "chromium.raw")

}  // namespace logging
