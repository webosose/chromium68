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

#include "components/discardable_memory/common/switches.h"

namespace discardable_memory {
namespace switches {

// Minimal memory limit to apply in face of moderate to critical memory
// pressure in MB.
const char kSharedMemMinimalLimitMB[] = "shared-mem-minimal-limit-mb";

// Divider factor to apply when calculating the memory limit in face of
// moderate memory pressure.
const char kSharedMemPressureDivider[] = "shared-mem-pressure-divider";

// Reduction factor to apply to overall system memory when calculating the
// default memory limit to use for discardable memory.
const char kSharedMemSystemMemReductionFactor[] =
    "shared-mem-system-mem-reduction-factor";

}  // namespace switches
}  // namespace discardable_memory
