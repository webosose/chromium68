// Copyright 2018-2019 LG Electronics, Inc.
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

#ifndef GIN_NEVA_V8_ISOLATE_MEMORY_TRACE_PROVIDER_H_
#define GIN_NEVA_V8_ISOLATE_MEMORY_TRACE_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "base/trace_event/neva/memory_trace/memory_trace_provider.h"
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
