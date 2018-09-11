// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/neva/malloc_trace_provider.h"
#include "base/trace_event/neva/memory_trace_manager.h"

#include <malloc.h>
#include <stddef.h>

#include "base/allocator/allocator_extension.h"
#include "base/allocator/allocator_shim.h"
#include "build/build_config.h"

namespace base {
namespace trace_event {
namespace neva {

// static
MallocTraceProvider* MallocTraceProvider::GetInstance() {
  return Singleton<MallocTraceProvider,
                   LeakySingletonTraits<MallocTraceProvider>>::get();
}

MallocTraceProvider::MallocTraceProvider() {}

MallocTraceProvider::~MallocTraceProvider() {}

bool MallocTraceProvider::OnMemoryTrace() {
  const char* print_fmt;
  size_t total_virtual_size = 0;
  size_t resident_size = 0;
  size_t allocated_objects_size = 0;

  struct mallinfo info = mallinfo();
  DCHECK_GE(info.arena + info.hblkhd, info.uordblks);

  MemoryTraceManager* mtm = MemoryTraceManager::GetInstance();

  // In case of Android's jemalloc |arena| is 0 and the outer pages size is
  // reported by |hblkhd|. In case of dlmalloc the total is given by
  // |arena| + |hblkhd|. For more details see link: http://goo.gl/fMR8lF.
  total_virtual_size = (info.arena + info.hblkhd) / KB;
  resident_size = info.uordblks / KB;
  allocated_objects_size = info.uordblks / KB;

  bool is_trace_log_csv = mtm->IsTraceLogCSV();

  if (!is_trace_log_csv) {
    print_fmt = "[malloc] virtual = %8zd KB, resident = %8zd KB, "
                         "allocated = %8zd KB\n";
  } else {
    print_fmt = "%zd, %zd, %zd";
  }
  if (mtm->GetUseMegaBytes()) {
    total_virtual_size = ConvertKBtoMB(total_virtual_size);
    resident_size = ConvertKBtoMB(resident_size);
    allocated_objects_size = ConvertKBtoMB(allocated_objects_size);
    if (!is_trace_log_csv)
      print_fmt = "[malloc] virtual = %4zd MB, resident = %4zd MB, "
                           "allocated = %4zd MB\n";
  }
  fprintf(mtm->GetTraceFile(), print_fmt,
          total_virtual_size, resident_size, allocated_objects_size);

  return true;
}

std::string MallocTraceProvider::GetCSVHeader() {
  return std::string("malloc:virtual, malloc:resident, malloc:allocated");
}

}  // namespace neva
}  // namespace trace_event
}  // namespace base
