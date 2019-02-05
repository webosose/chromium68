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

#include "content/public/common/content_neva_switches.h"

namespace switches {

// Enable aggressive GC on foreground tabs
const char kEnableAggressiveForegroundGC[] = "enable-aggressive-foreground-gc";

// Loads browser control injection
const char kEnableBrowserControlInjection[]
    = "enable-browser-control-injection";

// Allow making a File System according to File API: Directories and System.
const char kEnableFileAPIDirectoriesAndSystem[] =
    "enable-file-api-directories-and-system";

// Loads sample injection
const char kEnableSampleInjection[]     = "enable-sample-injection";

// Overrides the timeout, in seconds, that FirstMeaningfulPaintDetector waits
// for a network stable timer to be fired.
const char kNetworkStableTimeout[] = "network-stable-timeout";

// Specifies a list of hosts for whom we bypass proxy settings and use direct
// connections. Ignored if --proxy-auto-detect or --no-proxy-server are also
// specified. This is a comma-separated list of bypass rules. See:
// "net/proxy/proxy_bypass_rules.h" for the format of these rules.
// Named after kNevaProxyBypassList instead of kProxyBypassList to avoid
// name collision with chrome switches kProxyBypassList when using jumbo builds.
const char kNevaProxyBypassList[] = "proxy-bypass-list";

// Use platform implementation for Input Controls (File picker,
// Color chooser, e.t.c.)
const char kUseExternalInputControls[]  = "use-external-input-controls";

}  // namespace switches
