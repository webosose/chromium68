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

#ifndef EMULATOR_EMULATOR_URLS_H_
#define EMULATOR_EMULATOR_URLS_H_

#include "emulator/emulator_export.h"

namespace emulator {

EMULATOR_EXPORT extern const char kEmulatorDefaultHost[];
EMULATOR_EXPORT extern const int  kEmulatorDefaultPort;
EMULATOR_EXPORT extern const char kEmulatorExpectationPath[];

// All constants below should have two parts divided by underscore
// The first part is interface name, the second one is the interface method
// name

// Native Controls API
//      Color chooser
EMULATOR_EXPORT extern const char kNativeControls_changedColorChooser[];
EMULATOR_EXPORT extern const char kNativeControls_closeColorChooser[];
EMULATOR_EXPORT extern const char kNativeControls_closedColorChooser[];
EMULATOR_EXPORT extern const char kNativeControls_openColorChooser[];

EMULATOR_EXPORT extern const char kNativeControls_openColorChooser[];
//      File chooser
EMULATOR_EXPORT extern const char kNativeControls_retFileChooser[];
EMULATOR_EXPORT extern const char kNativeControls_runFileChooser[];

// Native Dialogs API
EMULATOR_EXPORT extern const char kNativeDialogs_cancelJavaScriptDialog[];
EMULATOR_EXPORT extern const char kNativeDialogs_retJavaScriptDialog[];
EMULATOR_EXPORT extern const char kNativeDialogs_runJavaScriptDialog[];

// Sample Dialogs API
EMULATOR_EXPORT extern const char kSample_callFunc[];
EMULATOR_EXPORT extern const char kSample_getPlatformValue[];
EMULATOR_EXPORT extern const char kSample_processDataResponse[];
EMULATOR_EXPORT extern const char kSample_processDataReq[];
EMULATOR_EXPORT extern const char kSample_sampleUpdate[];

// WAM API
EMULATOR_EXPORT extern const char kWam_callFunc[];
EMULATOR_EXPORT extern const char kWam_commandSet[];

EMULATOR_EXPORT extern const char kBrowserControl_callFunction[];
EMULATOR_EXPORT extern const char kBrowserControl_sendCommand[];


}  // namespace emulator

#endif  // EMULATOR_EMULATOR_URLS_H_
