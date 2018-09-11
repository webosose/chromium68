"use strict";

// These constant values shall be exactly the same as defined within
// neva/pc_impl/pal/common/emulator_urls.cc

const kHost = window.location.hostname;
const kPort = window.location.port;
const kEmulatorBaseURL = "http://" + kHost + ":" + kPort + "/";

const kEmulatorExpectationPath = "expectation";

// Native Controls API
//      Color chooser
const kNativeControls_changedColorChooser =
    "NativeControls/changedColorChooser";
const kNativeControls_closeColorChooser = "NativeControls/closeColorChooser";
const kNativeControls_closedColorChooser = "NativeControls/closedColorChooser";
const kNativeControls_openColorChooser = "NativeControls/openColorChooser";
//      File chooser
const kNativeControls_retFileChooser = "NativeControls/retFileChooser";
const kNativeControls_runFileChooser = "NativeControls/runFileChooser";

// Native Dialogs API
const kNativeDialogs_cancelJavaScriptDialog =
    "NativeDialogs/cancelJavaScriptDialog";
const kNativeDialogs_retJavaScriptDialog = "NativeDialogs/retJavaScriptDialog";
const kNativeDialogs_runJavaScriptDialog = "NativeDialogs/runJavaScriptDialog";

// Sample Dialogs API
const kSample_callFunc = "Sample/callFunc";
const kSample_getPlatformValue = "Sample/getPlatformValue";
const kSample_processDataResponse = "Sample/processDataResponse";
const kSample_processDataReq = "Sample/processDataReq";
const kSample_sampleUpdate = "Sample/sampleUpdate";

// WAM
const kWAM_callFunc = "WAM/callFunc";
const kWAM_commandSet = "WAM/commandSet";

// Browser Control API
const kBrowserControl_callFunction = "BrowserControl/callFunction"
const kBrowserControl_sendCommand = "BrowserControl/sendCommand"
