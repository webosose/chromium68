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

#include "emulator_urls.h"

namespace emulator {

const char kEmulatorDefaultHost[]         = "localhost";
const int  kEmulatorDefaultPort           = 8888;
const char kEmulatorExpectationPath[]     = "expectation";

// Native Controls API
//      Color chooser
const char kNativeControls_changedColorChooser[] =
    "NativeControls/changedColorChooser";
const char kNativeControls_closeColorChooser[] =
    "NativeControls/closeColorChooser";
const char kNativeControls_closedColorChooser[] =
    "NativeControls/closedColorChooser";
const char kNativeControls_openColorChooser[] =
    "NativeControls/openColorChooser";
//      File chooser
const char kNativeControls_retFileChooser[] = "NativeControls/retFileChooser";
const char kNativeControls_runFileChooser[] = "NativeControls/runFileChooser";

// Native Dialogs API
const char kNativeDialogs_cancelJavaScriptDialog[] =
    "NativeDialogs/cancelJavaScriptDialog";
const char kNativeDialogs_retJavaScriptDialog[] =
    "NativeDialogs/retJavaScriptDialog";
const char kNativeDialogs_runJavaScriptDialog[] =
    "NativeDialogs/runJavaScriptDialog";

// Sample Dialogs API
const char kSample_callFunc[]             = "Sample/callFunc";
const char kSample_getPlatformValue[]     = "Sample/getPlatformValue";
const char kSample_processDataResponse[]  = "Sample/processDataResponse";
const char kSample_processDataReq[]       = "Sample/processDataReq";
const char kSample_sampleUpdate[]         = "Sample/sampleUpdate";

// WAM API
const char kWam_callFunc[]                = "WAM/callFunc";
const char kWam_commandSet[]              = "WAM/commandSet";

const char kBrowserControl_callFunction[] = "BrowserControl/callFunction";
const char kBrowserControl_sendCommand[]  = "BrowserControl/sendCommand";

}  // namespace emulator
