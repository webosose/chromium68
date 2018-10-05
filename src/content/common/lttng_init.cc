// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#include "content/common/lttng_init.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"

namespace content {

void LttngInit() {
  base::FilePath module_dir;
#if defined(USE_CBE)
  CHECK(base::PathService::Get(base::DIR_MODULE, &module_dir));
#else   // !defined(USE_CBE)
  CHECK(base::PathService::Get(base::DIR_EXE, &module_dir));
#endif  // defined(USE_CBE)
  base::FilePath kLttngModulePath(
      FILE_PATH_LITERAL("libchromium_lttng_provider.so"));
  base::FilePath provider_path(module_dir.Append(kLttngModulePath).value());

  if (!base::PathExists(provider_path)) {
    VLOG(1) << "LTTng provider library does not exist at " << provider_path;
    return;
  }

  base::NativeLibraryLoadError error;
  base::NativeLibrary library = base::LoadNativeLibrary(provider_path, &error);
  VLOG_IF(1, !library) << "Unable to load LTTng provider from " << provider_path
                       << ": " << error.ToString();

  (void)library;  // Prevent release-mode warning.
}

}  // namespace content
