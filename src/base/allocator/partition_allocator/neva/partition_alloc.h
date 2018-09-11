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

#ifndef BASE_ALLOCATOR_PARTITION_ALLOCATOR_NEVA_PARTITION_ALLOC_H
#define BASE_ALLOCATOR_PARTITION_ALLOCATOR_NEVA_PARTITION_ALLOC_H

#include "base/allocator/partition_allocator/partition_alloc.h"

namespace base {
namespace neva {

// Interface that is passed to PartitionTraceStats and
// PartitionTraceStatsGeneric for using the memory statistics.
class BASE_EXPORT PartitionStatsTracer {
 public:
  // Called to trace total memory used by partition, once per partition.
  virtual void PartitionTraceTotals(const char* partition_name,
                                    const PartitionMemoryStats*) = 0;
};

BASE_EXPORT void PartitionTraceStats(PartitionRoot*,
                                     const char* partition_name,
                                     PartitionStatsTracer*);
BASE_EXPORT void PartitionTraceStatsGeneric(PartitionRootGeneric*,
                                            const char* partition_name,
                                            PartitionStatsTracer*);

}  // namespace neva
}  // namespace base

#endif  // BASE_ALLOCATOR_PARTITION_ALLOCATOR_NEVA_PARTITION_ALLOC_H
