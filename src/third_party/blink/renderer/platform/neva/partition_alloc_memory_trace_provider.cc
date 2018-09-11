// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "platform/neva/PartitionAllocMemoryTraceProvider.h"

#include <unordered_map>

#include "base/strings/stringprintf.h"
#include "base/trace_event/neva/memory_trace_manager.h"
#include "platform/wtf/allocator/Partitions.h"
#include "platform/wtf/text/WTFString.h"

namespace blink {

namespace {

using namespace WTF;

static constexpr int KB = 1024;

// This class is used to invert the dependency of PartitionAlloc on the
// PartitionAllocMemoryTraceProvider. This implements an interface that will
// be called with memory statistics for each bucket in the allocator.
class PartitionStatsTracerImpl final : public WTF::PartitionStatsTracer {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(PartitionStatsTracerImpl);
 public:
  PartitionStatsTracerImpl() : total_active_bytes_(0) {}

  // PartitionStatsTracer implementation.
  void PartitionTraceTotals(const char* partition_name,
                            const PartitionMemoryStats*) override;

  size_t TotalActiveBytes() const { return total_active_bytes_; }

 private:
  size_t total_active_bytes_;
};

void PartitionStatsTracerImpl::PartitionTraceTotals(
    const char* partition_name,
    const PartitionMemoryStats* memory_stats) {
  const char* print_fmt;
  base::trace_event::neva::MemoryTraceManager* mtm =
    base::trace_event::neva::MemoryTraceManager::GetInstance();
  FILE* trace_fp = mtm->GetTraceFile();
  bool is_trace_log_csv = mtm->IsTraceLogCSV();
  bool use_mega_bytes = mtm->GetUseMegaBytes();

  total_active_bytes_ += memory_stats->total_active_bytes;

  size_t total_resident_bytes = memory_stats->total_resident_bytes / KB;
  size_t total_active_bytes = memory_stats->total_active_bytes / KB;
  size_t total_mmapped_bytes = memory_stats->total_mmapped_bytes / KB;
  size_t total_committed_bytes = memory_stats->total_committed_bytes / KB;
  size_t total_decommittable_bytes = memory_stats->total_decommittable_bytes / KB;
  size_t total_discardable_bytes = memory_stats->total_discardable_bytes / KB;

  if (!is_trace_log_csv) {
    fprintf(trace_fp, "[PA/%12s] ", partition_name);
    print_fmt = "resident = %6zd, alloc = %6zd, mmap = %6zd, "
                "commit = %6zd, decommitable = %4zd, discardable = %6zd\n";
  } else {
    print_fmt = "%zd, %zd, %zd, %zd, %zd, %zd, ";
  }
  if (use_mega_bytes) {
    total_resident_bytes = ConvertKBtoMB(total_resident_bytes);
    total_active_bytes = ConvertKBtoMB(total_active_bytes);
    total_mmapped_bytes = ConvertKBtoMB(total_mmapped_bytes);
    total_committed_bytes = ConvertKBtoMB(total_committed_bytes);
    total_decommittable_bytes = ConvertKBtoMB(total_decommittable_bytes);
    total_discardable_bytes = ConvertKBtoMB(total_discardable_bytes);
    if (!is_trace_log_csv) {
      print_fmt = "resident = %3zd, alloc = %3zd, mmap = %3zd, "
                  "commit = %3zd, decommitable = %2zd, discardable = %3zd\n";
    }
  }
  fprintf(trace_fp, print_fmt,
          total_resident_bytes, total_active_bytes,
          total_mmapped_bytes, total_committed_bytes,
          total_decommittable_bytes, total_discardable_bytes);
}

} // namespace

namespace neva {

PartitionAllocMemoryTraceProvider* PartitionAllocMemoryTraceProvider::Instance() {
  DEFINE_STATIC_LOCAL(PartitionAllocMemoryTraceProvider, instance, ());
  return &instance;
}

bool PartitionAllocMemoryTraceProvider::OnMemoryTrace() {
  PartitionStatsTracerImpl partition_stats_tracer;
  const char* print_fmt;
  base::trace_event::neva::MemoryTraceManager* mtm =
    base::trace_event::neva::MemoryTraceManager::GetInstance();
  FILE* trace_fp = mtm->GetTraceFile();
  bool is_trace_log_csv = mtm->IsTraceLogCSV();
  bool use_mega_bytes = mtm->GetUseMegaBytes();

  // This method calls memory_stats.partitionsTraceBucketStats with memory statistics.
  WTF::Partitions::TraceMemoryStats(&partition_stats_tracer);

  size_t total_active_bytes = partition_stats_tracer.TotalActiveBytes() / KB;
  if (!is_trace_log_csv) {
    print_fmt = "[Partition Alloc] total active (alloced) = %8zd\n";
  } else {
    print_fmt = "%zd";
  }
  if (use_mega_bytes) {
    total_active_bytes = ConvertKBtoMB(total_active_bytes);
  }
  fprintf(trace_fp, print_fmt, total_active_bytes);

  return true;
}

std::string PartitionAllocMemoryTraceProvider::GetCSVHeader() {
  std::string header;
  std::vector<std::string> v = { std::string("fast_malloc"),
                                 std::string("array_buffer"),
                                 std::string("buffer"),
                                 std::string("layout") };
  for (auto& s : v) {
    header += s + ":resident, ";
    header += s + ":alloc, ";
    header += s + ":mmap, ";
    header += s + ":commit, ";
    header += s + ":decommitable, ";
    header += s + ":discardable, ";
  }
  header += "partition:total_active";
  return header;
}

}  // namespace neva

}  // namespace blink
