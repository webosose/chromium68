// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_NEVA_MALLOC_TRACE_PROVIDER_H_
#define BASE_TRACE_EVENT_NEVA_MALLOC_TRACE_PROVIDER_H_

#include <istream>
#include <memory>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/threading/platform_thread.h"
#include "base/trace_event/neva/memory_trace_provider.h"
#include "build/build_config.h"

namespace base {
namespace trace_event {
namespace neva {

// Trace provider which collects process-wide memory stats.
class BASE_EXPORT MallocTraceProvider : public MemoryTraceProvider {
 public:
  static MallocTraceProvider* GetInstance();

  // MemoryTraceProvider implementation.
  bool OnMemoryTrace() override;

  std::string GetCSVHeader() override;

 private:
  friend struct DefaultSingletonTraits<MallocTraceProvider>;

  MallocTraceProvider();
  ~MallocTraceProvider() override;

  DISALLOW_COPY_AND_ASSIGN(MallocTraceProvider);
};

}  // namespace neva
}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_NEVA_MALLOC_TRACE_PROVIDER_H_
