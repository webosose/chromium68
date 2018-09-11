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

#include "base/allocator/partition_allocator/partition_alloc.h"
#include "base/allocator/partition_allocator/neva/partition_alloc.h"

namespace base {

void PartitionDumpBucketStats(PartitionBucketMemoryStats* stats_out,
                              const PartitionBucket* bucket);

namespace neva {

void PartitionTraceStatsGeneric(PartitionRootGeneric* partition,
                                const char* partition_name,
                                PartitionStatsTracer* tracer) {
  PartitionMemoryStats stats = {0};
  stats.total_mmapped_bytes = partition->total_size_of_super_pages +
                              partition->total_size_of_direct_mapped_pages;
  stats.total_committed_bytes = partition->total_size_of_committed_pages;

  size_t direct_mapped_allocations_total_size = 0;

  static const size_t kMaxReportableDirectMaps = 4096;

  PartitionBucketMemoryStats bucket_stats[kGenericNumBuckets];
  size_t num_direct_mapped_allocations = 0;
  {
    subtle::SpinLock::Guard guard(partition->lock);

    for (size_t i = 0; i < kGenericNumBuckets; ++i) {
      const PartitionBucket* bucket = &partition->buckets[i];
      // Don't report the pseudo buckets that the generic allocator sets up in
      // order to preserve a fast size->bucket map (see
      // PartitionAllocGenericInit for details).
      if (!bucket->active_pages_head)
        bucket_stats[i].is_valid = false;
      else
        base::PartitionDumpBucketStats(&bucket_stats[i], bucket);
      if (bucket_stats[i].is_valid) {
        stats.total_resident_bytes += bucket_stats[i].resident_bytes;
        stats.total_active_bytes += bucket_stats[i].active_bytes;
        stats.total_decommittable_bytes += bucket_stats[i].decommittable_bytes;
        stats.total_discardable_bytes += bucket_stats[i].discardable_bytes;
      }
    }

    for (PartitionDirectMapExtent *extent = partition->direct_map_list;
         extent && num_direct_mapped_allocations < kMaxReportableDirectMaps;
         extent = extent->next_extent, ++num_direct_mapped_allocations) {
      DCHECK(!extent->next_extent ||
             extent->next_extent->prev_extent == extent);
      size_t slot_size = extent->bucket->slot_size;
      direct_mapped_allocations_total_size += slot_size;
    }
  }
  stats.total_resident_bytes += direct_mapped_allocations_total_size;
  stats.total_active_bytes += direct_mapped_allocations_total_size;
  tracer->PartitionTraceTotals(partition_name, &stats);
}

void PartitionTraceStats(PartitionRoot* partition,
                         const char* partition_name,
                         PartitionStatsTracer* tracer) {

  PartitionMemoryStats stats = {0};
  stats.total_mmapped_bytes = partition->total_size_of_super_pages;
  stats.total_committed_bytes = partition->total_size_of_committed_pages;
  DCHECK(!partition->total_size_of_direct_mapped_pages);

  static const size_t kMaxReportableBuckets = 4096 / sizeof(void*);

  const size_t partitionNumBuckets = partition->num_buckets;
  DCHECK(partitionNumBuckets <= kMaxReportableBuckets);

  for (size_t i = 0; i < partitionNumBuckets; ++i) {
    PartitionBucketMemoryStats bucket_stats = {0};
    base::PartitionDumpBucketStats(&bucket_stats, &partition->buckets()[i]);
    if (bucket_stats.is_valid) {
      stats.total_resident_bytes += bucket_stats.resident_bytes;
      stats.total_active_bytes += bucket_stats.active_bytes;
      stats.total_decommittable_bytes += bucket_stats.decommittable_bytes;
      stats.total_discardable_bytes += bucket_stats.discardable_bytes;
    }
  }
  tracer->PartitionTraceTotals(partition_name, &stats);
}

}  // namespace neva
}  // namespace base
