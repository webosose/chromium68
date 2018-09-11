// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_PROVIDER_H_
#define BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_PROVIDER_H_

#include <string>

#include "base/base_export.h"
#include "base/macros.h"

namespace base {
namespace trace_event {
namespace neva {

#define ConvertKBtoMB(kb) ((kb) / 1024)

static constexpr int KB = 1024;

// The contract interface that memory trace providers must implement.
class BASE_EXPORT MemoryTraceProvider {
 public:
  virtual ~MemoryTraceProvider() {}

  virtual bool OnMemoryTrace() = 0;

  virtual std::string GetCSVHeader() = 0;

 protected:
  MemoryTraceProvider() {}

  DISALLOW_COPY_AND_ASSIGN(MemoryTraceProvider);
};

}  // namespace neva
}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_PROVIDER_H_
