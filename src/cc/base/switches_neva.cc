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

#include "cc/base/switches_neva.h"

namespace cc {
namespace switches {

// Layer tree setting for decoded image working set budget in MB
const char kDecodedImageWorkingSetBudgetMB[] =
    "decoded-image-working-set-budget-mb";

// Enables using aggressive release policy to reclaim allocated resources
// when application goes into hidden state.
const char kEnableAggressiveReleasePolicy[] =
    "enable-aggressive-release-policy";

// Enable native-scroll feature
const char kEnableWebOSNativeScroll[] = "enable-native-scroll";

// The factor by which to reduce the GPU memory size of the cache when under
// memory pressure.
const char kMemPressureGPUCacheSizeReductionFactor[] =
    "mem-pressure-gpu-cache-size-reduction-factor";

// The factor by which to reduce the tile manager low memory policy bytes
// limit when under memory pressure.
const char kTileManagerLowMemPolicyBytesLimitReductionFactor[] =
    "tile-manager-low-mem-policy-bytes-limit-reduction-factor";

}  // namespace switches
}  // namespace cc
