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

#include "gin/neva/v8_isolate_memory_trace_provider.h"

#include <inttypes.h>
#include <stddef.h>

#include "base/strings/stringprintf.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/neva/memory_trace/memory_trace_manager.h"
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
  int mb = use_mega_bytes ? 1024 : 1;

  v8::HeapStatistics heap_statistics;
  isolate_holder_->isolate()->GetHeapStatistics(&heap_statistics);

  size_t malloced_memory = heap_statistics.malloced_memory() / KB / mb;
  size_t peak_malloced_memory = heap_statistics.peak_malloced_memory() / KB / mb;

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
    fprintf(trace_fp, print_fmt, malloced_memory, peak_malloced_memory);
  } else {
    // Do not print v8 malloced info in csv mode.
  }

  // Trace statistics of each space in v8 heap.
  size_t known_spaces_used_size = 0;
  size_t known_spaces_size = 0;
  size_t known_spaces_physical_size = 0;
  size_t number_of_spaces = isolate_holder_->isolate()->NumberOfHeapSpaces();
  for (size_t space = 0; space < number_of_spaces; space++) {
    v8::HeapSpaceStatistics space_statistics;
    isolate_holder_->isolate()->GetHeapSpaceStatistics(&space_statistics,
                                                       space);

    size_t space_physical_size = space_statistics.physical_space_size() / KB / mb;
    size_t space_size = space_statistics.space_size() / KB / mb;
    size_t space_used_size = space_statistics.space_used_size() / KB / mb;

    if (is_trace_log_csv) {
      fprintf(trace_fp, ", %zd", space_used_size);
    } else {
      const char* name = space_statistics.space_name();
      if (!strcmp(name, "large_object_space"))
        name = "lo_space";
      fprintf(trace_fp, "[v8/%12s] ", name);
      print_fmt = "physical = %6zd, virtual = %6zd, allocated = %6zd\n";

      fprintf(trace_fp, print_fmt,
              space_physical_size, space_size, space_used_size);
    }

    known_spaces_size += space_size;
    known_spaces_used_size += space_used_size;
    known_spaces_physical_size += space_physical_size;
  }

  // Compute the rest of the memory, not accounted by the spaces above.
  size_t total_physical_size = heap_statistics.total_physical_size() / KB / mb;
  size_t total_heap_size = heap_statistics.total_heap_size() / KB / mb;
  size_t used_heap_size = heap_statistics.used_heap_size() / KB / mb;

  if (is_trace_log_csv) {
    fprintf(trace_fp, ", %zd", (used_heap_size - known_spaces_used_size));
  } else {
    print_fmt =
        "[v8/      others] physical = %6zd, virtual = %6zd, allocated = %6zd\n";
    fprintf(trace_fp, print_fmt,
            (total_physical_size - known_spaces_physical_size),
            (total_heap_size - known_spaces_size),
            (used_heap_size - known_spaces_used_size));
  }
}

std::string V8IsolateMemoryTraceProvider::GetCSVHeader() {
  std::string header = "v8:isolate";
  size_t number_of_spaces = isolate_holder_->isolate()->NumberOfHeapSpaces();

  for (size_t space = 0; space < number_of_spaces; space++) {
    v8::HeapSpaceStatistics space_statistics;
    isolate_holder_->isolate()->GetHeapSpaceStatistics(&space_statistics,
                                                       space);
    const char* space_name = space_statistics.space_name();
    header += std::string(", v8:") + space_name;
  }
  header += ", v8:other";

  return header;
}

}  // namespace neva
}  // namespace gin
