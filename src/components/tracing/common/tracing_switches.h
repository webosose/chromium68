// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_TRACING_COMMON_TRACING_SWITCHES_H_
#define COMPONENTS_TRACING_COMMON_TRACING_SWITCHES_H_

#include "components/tracing/tracing_export.h"

namespace switches {

TRACING_EXPORT extern const char kTraceConfigFile[];
TRACING_EXPORT extern const char kTraceShutdown[];
TRACING_EXPORT extern const char kTraceShutdownFile[];
TRACING_EXPORT extern const char kTraceStartup[];
TRACING_EXPORT extern const char kTraceStartupDuration[];
TRACING_EXPORT extern const char kTraceStartupFile[];
TRACING_EXPORT extern const char kTraceStartupRecordMode[];
TRACING_EXPORT extern const char kTraceToConsole[];
TRACING_EXPORT extern const char kTraceUploadURL[];

#if defined(USE_MEMORY_TRACE)
TRACING_EXPORT extern const char kTraceMemoryBrowser[];
TRACING_EXPORT extern const char kTraceMemoryRenderer[];
TRACING_EXPORT extern const char kTraceMemoryInterval[];
TRACING_EXPORT extern const char kTraceMemoryToFile[];
TRACING_EXPORT extern const char kTraceMemoryLogFormat[];
TRACING_EXPORT extern const char kTraceMemoryByteUnit[];
#endif

}  // namespace switches

#endif  // COMPONENTS_TRACING_COMMON_TRACING_SWITCHES_H_
