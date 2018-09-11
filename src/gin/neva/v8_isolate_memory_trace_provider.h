// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef GIN_NEVA_V8_ISOLATE_MEMORY_TRACE_PROVIDER_H_
#define GIN_NEVA_V8_ISOLATE_MEMORY_TRACE_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "base/trace_event/neva/memory_trace_provider.h"
#include "gin/gin_export.h"

namespace gin {

class IsolateHolder;

namespace neva {

// Memory trace provider for the chrome://tracing infrastructure. It traces
// summarized memory stats about the V8 Isolate.
class V8IsolateMemoryTraceProvider
    : public base::trace_event::neva::MemoryTraceProvider {
 public:
  explicit V8IsolateMemoryTraceProvider(gin::IsolateHolder* isolate_holder);
  ~V8IsolateMemoryTraceProvider() override;

  // MemoryTraceProvider implementation.
  bool OnMemoryTrace() override;

 private:
  void TraceHeapStatistics();

  std::string GetCSVHeader() override;

  gin::IsolateHolder* isolate_holder_;  // Not owned.

  DISALLOW_COPY_AND_ASSIGN(V8IsolateMemoryTraceProvider);
};

}  // namespace neva
}  // namespace gin

#endif  // GIN_NEVA_V8_ISOLATE_MEMORY_TRACE_PROVIDER_H_
