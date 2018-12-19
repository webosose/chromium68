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

#ifndef BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_SYSTEM_TRACE_PROVIDER_H_
#define BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_SYSTEM_TRACE_PROVIDER_H_

#include <istream>
#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/neva/memory_trace/memory_trace_provider.h"
#include "build/build_config.h"

namespace base {
namespace trace_event {
namespace neva {

// Trace provider which collects process-wide memory stats.
class BASE_EXPORT SystemTraceProvider : public MemoryTraceProvider {
 public:
  static SystemTraceProvider* GetInstance();

  // MemoryTraceProvider implementation.
  bool OnMemoryTrace() override;

  std::string GetCSVHeader() override;

 private:
  friend struct DefaultSingletonTraits<SystemTraceProvider>;

  SystemTraceProvider();
  ~SystemTraceProvider() override;

  DISALLOW_COPY_AND_ASSIGN(SystemTraceProvider);
};

}  // namespace neva
}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_SYSTEM_TRACE_PROVIDER_H_
