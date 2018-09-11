// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#include "app_runtime_browser_switches.h"

// Allow file:// access if specified
const char kAllowFileAccess[] = "allow-file-access";

// Forces the maximum disk space to be used by the disk cache, in bytes.
const char kDiskCacheSize[] = "disk-cache-size";

// A string used to override the default user agent with a custom one.
const char kUserAgent[] = "user-agent";

// Use a specific location for the directory which contains data specific
// to a given user.
const char kUserDataDir[] = "user-data-dir";

// If true devtools experimental settings are enabled
const char kEnableDevToolsExperiments[] = "enable-devtools-experiments";
