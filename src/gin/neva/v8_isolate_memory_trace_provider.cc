// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "gin/neva/v8_isolate_memory_trace_provider.h"

#include <inttypes.h>
#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/neva/memory_trace_manager.h"
#include "gin/public/isolate_holder.h"
#include "v8/include/v8.h"

static const int KB = 1024;

namespace gin {
namespace neva {

V8IsolateMemoryTraceProvider::V8IsolateMemoryTraceProvider(
    IsolateHolder* isolate_holder)
    : isolate_holder_(isolate_holder) {
  base::trace_event::neva::MemoryTraceManager::GetInstance()
      ->RegisterTraceProvider(this, "V8Isolate",
                              base::ThreadTaskRunnerHandle::Get());
}

V8IsolateMemoryTraceProvider::~V8IsolateMemoryTraceProvider() {
  base::trace_event::neva::MemoryTraceManager::GetInstance()
      ->UnregisterTraceProvider(this);
}

// Called at trace dump point time. Creates a snapshot with the memory counters
// for the current isolate.
bool V8IsolateMemoryTraceProvider::OnMemoryTrace() {
  if (isolate_holder_->access_mode() == IsolateHolder::kUseLocker) {
    v8::Locker locked(isolate_holder_->isolate());
    TraceHeapStatistics();
  } else {
    TraceHeapStatistics();
  }
  return true;
}

void V8IsolateMemoryTraceProvider::TraceHeapStatistics() {
  const char* print_fmt;
  base::trace_event::neva::MemoryTraceManager* mtm =
    base::trace_event::neva::MemoryTraceManager::GetInstance();
  FILE* trace_fp = mtm->GetTraceFile();
  bool is_trace_log_csv = mtm->IsTraceLogCSV();
  bool use_mega_bytes = mtm->GetUseMegaBytes();

  v8::HeapStatistics heap_statistics;
  isolate_holder_->isolate()->GetHeapStatistics(&heap_statistics);
  size_t malloced_memory = heap_statistics.malloced_memory() / KB;
  size_t peak_malloced_memory = heap_statistics.peak_malloced_memory() / KB;

  // Print isolate information.
  if (!is_trace_log_csv) {
    print_fmt = "[v8/     isolate] %p\n";
  } else {
    print_fmt = "%p";
  }
  fprintf(trace_fp, print_fmt, isolate_holder_->isolate());

  // Trace statistics about malloced memory.
  if (!is_trace_log_csv) {
    print_fmt = "[v8/      malloc] malloced = %6zd, peak    = %6zd\n";
  } else {
    print_fmt = ", %zd, %zd";
  }
  if (use_mega_bytes) {
    malloced_memory = ConvertKBtoMB(malloced_memory);
    peak_malloced_memory = ConvertKBtoMB(peak_malloced_memory);
  }
  fprintf(trace_fp, print_fmt, malloced_memory, peak_malloced_memory);

  // Trace statistics of each space in v8 heap.
  size_t known_spaces_used_size = 0;
  size_t known_spaces_size = 0;
  size_t known_spaces_physical_size = 0;
  size_t number_of_spaces = isolate_holder_->isolate()->NumberOfHeapSpaces();
  for (size_t space = 0; space < number_of_spaces; space++) {
    v8::HeapSpaceStatistics space_statistics;
    isolate_holder_->isolate()->GetHeapSpaceStatistics(&space_statistics,
                                                       space);

    size_t space_size = space_statistics.space_size() / KB;
    size_t space_used_size = space_statistics.space_used_size() / KB;
    size_t space_physical_size = space_statistics.physical_space_size() / KB;

    if (!is_trace_log_csv) {
      const char* name = space_statistics.space_name();
      if (!strcmp(name, "large_object_space"))
        name = "lo_space";
      fprintf(trace_fp, "[v8/%12s] ", name);
      print_fmt = "physical = %6zd, virtual = %6zd, allocated = %6zd\n";
    } else {
      print_fmt = ", %zd, %zd, %zd";
    }
    if (use_mega_bytes) {
      space_physical_size = ConvertKBtoMB(space_physical_size);
      space_size = ConvertKBtoMB(space_size);
      space_used_size = ConvertKBtoMB(space_used_size);
    }
    fprintf(trace_fp, print_fmt,
            space_physical_size, space_size, space_used_size);

    known_spaces_size += space_size;
    known_spaces_used_size += space_used_size;
    known_spaces_physical_size += space_physical_size;
  }

  // Compute the rest of the memory, not accounted by the spaces above.
  size_t total_physical_size = heap_statistics.total_physical_size() / KB;
  size_t total_heap_size = heap_statistics.total_heap_size() / KB;
  size_t used_heap_size = heap_statistics.used_heap_size() / KB;
  if (!is_trace_log_csv) {
    print_fmt =
        "[v8/      others] physical = %6zd, virtual = %6zd, allocated = %6zd\n";
  } else {
    print_fmt = ", %zd, %zd, %zd";
  }
  if (use_mega_bytes) {
    total_physical_size = ConvertKBtoMB(total_physical_size);
    total_heap_size = ConvertKBtoMB(total_heap_size);
    used_heap_size = ConvertKBtoMB(used_heap_size);
  }
  fprintf(trace_fp, print_fmt,
          (total_physical_size - known_spaces_physical_size),
          (total_heap_size - known_spaces_size),
          (used_heap_size - known_spaces_used_size));
}

std::string V8IsolateMemoryTraceProvider::GetCSVHeader() {
  std::string header = "v8:isolate, v8:malloc:size, v8:malloc:peak";
  size_t number_of_spaces = isolate_holder_->isolate()->NumberOfHeapSpaces();

  for (size_t space = 0; space < number_of_spaces; space++) {
    v8::HeapSpaceStatistics space_statistics;
    isolate_holder_->isolate()->GetHeapSpaceStatistics(&space_statistics,
                                                       space);
    const char* space_name = space_statistics.space_name();
    header += std::string(", v8:") + space_name + ":phy";
    header += std::string(", v8:") + space_name + ":virt";
    header += std::string(", v8:") + space_name + ":alloc";
  }
  header += ", v8:other:phy, v8:other:virt, v8:other:alloc";

  return header;
}

}  // namespace neva
}  // namespace gin
