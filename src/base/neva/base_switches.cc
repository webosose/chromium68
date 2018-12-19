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

#include "base/neva/base_switches.h"

namespace switches {

// V8 snapshot blob path
const char kV8SnapshotBlobPath[] = "v8-snapshot-blob-path";

#if defined(USE_MEMORY_TRACE)
const char kTraceMemoryBrowser[]   = "trace-memory-browser";
const char kTraceMemoryRenderer[]  = "trace-memory-renderer";
const char kTraceMemoryInterval[]  = "trace-memory-interval";
const char kTraceMemoryToFile[]    = "trace-memory-to-file";
const char kTraceMemoryLogFormat[] = "trace-memory-log-format";
const char kTraceMemoryByteUnit[]  = "trace-memory-byte-unit";
#endif

}  // namespace switches
