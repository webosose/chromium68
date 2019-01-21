// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "neva/wam_demo/wam_demo_service.h"

#include "base/base_paths.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/threading/thread_task_runner_handle.h"
#include "content/public/common/content_switches.h"
#include "emulator/emulator_urls.h"
#include "neva/app_runtime/public/app_runtime_constants.h"
#include "neva/app_runtime/webview_profile.h"
#include "neva/wam_demo/wam_demo_webapp.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#include "util.h"

#include <signal.h>
#include <sstream>

namespace wam_demo {

const char WamDemoService::wam_demo_app_prefix_[] = "com.webos.app.neva.wam.demo.";

namespace {

const int kDefaultWindowWidth = 640;
const int kDefaultWindowHeight = 480;

namespace command {

char kAddUserStyleSheet[] = "addUserStyleSheet";
char kAllowUniversalAccessFromFileUrls[] = "allowUniversalAccessFromFileUrls";
char kCanGoBack[] = "canGoBack";
char kChangeUrl[] = "changeURL";
char kClearInjections[] = "clearInjections";
char kClearMediaCapturePermission[] = "clearMediaCapturePermission";
char kDecidePolicyForResponse[] = "decidePolicyForResponse";
char kDeleteWebStorages[] = "deleteWebStorages";
char kDoNotTrack[] = "doNotTrack";
char kEnableInspectablePage[] = "enableInspectablePage";
char kExtraWebSocketHeader[] = "setAdditionalHeader";
char kForegroundApp[] = "foregroundApp";
char kGetDevToolsEndpoint[] = "getDevToolsEndpoint";
char kGetPid[] = "getPid";
char kGetUserAgent[] = "getUserAgent";
char kGetWindowState[] = "getWindowState";
char kGetWindowStateAboutToChange[] = "getWindowStateAboutToChange";
char kGoBack[] = "goBack";
char kHideApp[] = "hideApp";
char kIgnoreSSLError[] = "ignoreSSLError";
char kIsKeyboardVisible[] = "isKeyboardVisible";
char kKillApp[] = "killApp";
char kLaunchApp[] = "launchApp";
char kLaunchHiddenApp[] = "launchHiddenApp";
char kLoadInjections[] = "loadInjections";
char kReloadPage[] = "reloadPage";
char kReplaceBaseURL[] = "replaceBaseURL";
char kResetCompositorPainting[] = "resetCompositorPainting";
char kResizeWindow[] = "resizeWindow";
char kResumeDOM[] = "resumeDOM";
char kResumeMedia[] = "resumeMedia";
char kResumePainting[] = "resumePainting";
char kRunJavaScript[] = "runJavaScript";
char kRunJSInAllFrames[] = "runJSInAllFrames";
char kSetAcceptLanguage[] = "setAcceptLanguage";
char kSetAllowFakeBoldText[] = "setAllowFakeBoldText";
char kSetBackgroundColor[] = "setBackgroundColor";
char kSetBoardType[] = "setBoardType";
char kSetCustomCursor[] = "setCustomCursor";
char kSetFocus[] = "setFocus";
char kSetFontFamily[] = "setFontFamily";
char kSetGroupKeyMask[] = "setGroupKeyMask";
char kSetHardwareResolution[] = "setHardwareResolution";
char kSetHTMLSystemKeyboardEnabled[] = "setHTMLSystemKeyboardEnabled";
char kSetInputRegion[] = "setInputRegion";
char kSetInspectable[] = "setInspectable";
char kSetKeyMask[] = "setKeyMask";
char kSetMediaCapturePermission[] = "setMediaCapturePermission";
char kSetMediaCodecCapability[] = "setMediaCodecCapability";
char kSetOpacity[] = "setOpacity";
char kSetProxyServer[] = "setProxyServer";
char kSetScaleFactor[] = "setScaleFactor";
char kSetUseVirtualKeyboard[] = "setUseVirtualKeyboard";
char kSetUserAgent[] = "setUserAgent";
char kSetVisibilityState[] = "setVisibilityState";
char kSetWindowProperty[] = "setWindowProperty";
char kSetWindowState[] = "setWindowState";
char kShowApp[] = "showApp";
char kStopApp[] = "stopApp";
char kStopLoading[] = "stopLoading";
char kSuspendDOM[] = "suspendDOM";
char kSuspendMedia[] = "suspendMedia";
char kSuspendPainting[] = "suspendPainting";
char kUpdateAppWindow[] = "updateAppWindow";
char kUpdateZoom[] = "updateZoom";
char kXInputActivate[] = "xinputActivate";
char kXInputDeactivate[] = "xinputDeactivate";
char kXInputInvokeAction[] = "xinputInvokeAction";

}  // namespace command

namespace response {

char kAppClosed[] = "appClosed";
char kAppStarted[] = "appStarted";
char kDevToolsEndpoint[] = "devToolsEndpoint";
char kCanGoBackAbility[] = "canGoBackAbility";
char kKeyboardVisibility[] = "keyboardVisibility";
char kLoadFailed[] = "loadFailed";
char kLoadFinished[] = "loadFinished";
char kPidRequested[] = "pidRequested";
char kPidUpdated[] = "pidUpdated";
char kProcessGone[] = "processGone";
char kUserAgentIs[] = "userAgentIs";
char kWindowStateAboutToChangeRequested[] = "windowStateAboutToChangeRequested";
char kWindowStateRequested[] = "windowStateRequested";
char kZoomUpdated[] = "zoomUpdated";

}  // response

namespace argument {

char kAllow[] = "allow";
char kAppId[] = "app_id";
char kBlueColor[] = "blue";
char kBoardType[] = "board_type";
char kCmd[] = "cmd";
char kEnable[] = "enable";
char kFontFamily[] = "font_family";
char kFramelessWindow[] = "frameless_window";
char kFullScreen[] = "full_screen";
char kGreenColor[] = "green";
char kHeight[] = "height";
char kInputRegion[] = "input_region";
char kJSCode[] = "javascript_code";
char kKeyMask[] = "key_mask";
char kKeySym[] = "key_sym";
char kMediaCodecCapability[] = "media_codec_capability";
char kName[] = "name";
char kOpacity[] = "opacity";
char kPosX[] = "pos_x";
char kPosY[] = "pos_y";
char kRedColor[] = "red";
char kResolutionHeight[] = "resolution_height";
char kResolutionWidth[] = "resolution_width";
char kScaleFactor[] = "scale_factor";
char kSet[] = "set";
char kUrl[] = "url";
char kUserAgent[] = "user_agent";
char kUserStyleSheet[] = "user_stylesheet";
char kValue[] = "value";
char kViewportHeight[] = "viewport_height";
char kViewportWidth[] = "viewport_width";
char kVisibilityState[] = "visibility_state";
char kWidth[] = "width";
char kWindowHeight[] = "window_height";
char kWindowState[] = "window_state";
char kWindowWidth[] = "window_width";
char kZoomFactor[] = "zoom";
int kDefaultAlphaValue = 255;

}  // argument

bool UnpackGeneralParams(const std::string& src,
                         std::string& appid,
                         std::string& cmd,
                         std::string& appurl) {
  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kAppId, &appid});
  args_vector.push_back({argument::kCmd, &cmd});
  args_vector.push_back({argument::kUrl, &appurl});
  return emulator::EmulatorDataSource::GetResponseParams(args_vector, src);
}

bool UnpackLayoutParams(const std::string& src,
                        int& x, int& y, int& w, int& h) {
  std::string width;
  std::string height;
  std::string pos_x;
  std::string pos_y;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kWidth, &width});
  args_vector.push_back({argument::kHeight, &height});
  args_vector.push_back({argument::kPosX, &pos_x});
  args_vector.push_back({argument::kPosY, &pos_y});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(pos_x, &x) && base::StringToInt(pos_y, &y) &&
      base::StringToInt(width, &w) && base::StringToInt(height, &h);
}

bool UnpackInjections(const std::string& src,
                      std::vector<std::string>& injections) {
  emulator::ResponseArgs args_vector;
  std::string csv;
  args_vector.push_back({"injections", &csv});
  if (emulator::EmulatorDataSource::GetResponseParams(args_vector, src)) {
    std::vector<std::string> res = base::SplitString(
      csv, ",;", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
    if (!res.empty()) {
      injections = std::move(res);
      return true;
    }
  }
  return false;
}

bool UnpackBackgroundColor(const std::string& src, int& r, int& g, int& b) {
  std::string red;
  std::string green;
  std::string blue;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kRedColor, &red});
  args_vector.push_back({argument::kGreenColor, &green});
  args_vector.push_back({argument::kBlueColor, &blue});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(red, &r) && base::StringToInt(green, &g) &&
      base::StringToInt(blue, &b);
}

bool UnpackBool(const std::string& src, const char* name, bool& param) {
  std::string str_param;
  emulator::ResponseArgs args_vector;
  args_vector.push_back({name, &str_param});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  const std::string upper_str_param = base::ToUpperASCII(str_param);
  if (upper_str_param == std::string("TRUE")) {
    param = true;
    return true;
  }

  if (upper_str_param == std::string("FALSE")) {
    param = false;
    return true;
  }

  param = false;
  return false;
}

bool UnpackInt(const std::string& src, const char* name, int& param) {
  std::string str_param;
  emulator::ResponseArgs args_vector;
  args_vector.push_back({name, &str_param});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(str_param, &param);
}

bool UnpackFloat(const std::string& src, const char* name, float& param) {
  std::string str_param;
  emulator::ResponseArgs args_vector;
  args_vector.push_back({name, &str_param});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  double res = 0;
  if (base::StringToDouble(str_param, &res)) {
    param = static_cast<float>(res);
    return true;
  }

  return false;
}

bool UnpackString(const std::string& src, const char* name, std::string& param) {
  emulator::ResponseArgs args_vector;
  args_vector.push_back({name, &param});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  if (param.empty())
    return false;

  return true;
}

bool UnpackViewportParams(const std::string& src,
                          int& w, int& h) {
  std::string width;
  std::string height;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kViewportWidth, &width});
  args_vector.push_back({argument::kViewportHeight, &height});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(width, &w) && base::StringToInt(height, &h);
}

bool UnpackWindowSize(const std::string& src, int& w, int& h) {
  std::string width, height;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kWindowWidth, &width});
  args_vector.push_back({argument::kWindowHeight, &height});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(width, &w) && base::StringToInt(height, &h);
}

bool UnpackHardwareResolution(const std::string& src, int& w, int& h) {
  std::string width, height;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kResolutionWidth, &width});
  args_vector.push_back({argument::kResolutionHeight, &height});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  return base::StringToInt(width, &w) && base::StringToInt(height, &h);
}

bool GetWidgetStateFromInteger(int s, app_runtime::WidgetState& state) {
  bool result = true;
  switch (s) {
    case 3:
      state = app_runtime::WidgetState::FULLSCREEN;
      break;
    case 4:
      state = app_runtime::WidgetState::MAXIMIZED;
      break;
    case 5:
      state = app_runtime::WidgetState::MINIMIZED;
      break;
    default:
      state = app_runtime::WidgetState::UNINITIALIZED;
      result = false;
      break;
  }
  return result;
}

bool UnpackWindowState(const std::string& src, app_runtime::WidgetState& s) {
  std::string state;

  emulator::ResponseArgs args_vector;
  args_vector.push_back({argument::kWindowState, &state});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  int value = 0;

  return base::StringToInt(state, &value) && GetWidgetStateFromInteger(value, s);
}

const char* GetWidgetStateString(app_runtime::WidgetState state) {
  switch (state) {
    case app_runtime::WidgetState::SHOW:
      return "SHOW";
    case app_runtime::WidgetState::HIDE:
      return "HIDE";
    case app_runtime::WidgetState::FULLSCREEN:
      return "FULLSCREEN";
    case app_runtime::WidgetState::MAXIMIZED:
      return "MAXIMIZED";
    case app_runtime::WidgetState::MINIMIZED:
      return "MINIMIZED";
    case app_runtime::WidgetState::RESTORE:
      return "RESTORE";
    case app_runtime::WidgetState::ACTIVE:
      return "ACTIVE";
    case app_runtime::WidgetState::INACTIVE:
      return "INACTIVE";
    case app_runtime::WidgetState::RESIZE:
      return "RESIZE";
    case app_runtime::WidgetState::DESTROYED:
      return "DESTROYED";
    default:
      return "UNINITIALIZED";
  }
}

bool UnpackInputRegion(const std::string& src,
                       const char* name,
                       gfx::Rect& input_region,
                       int window_width,
                       int window_height) {
  std::string region;
  emulator::ResponseArgs args_vector;
  args_vector.push_back({name, &region});

  if (!emulator::EmulatorDataSource::GetResponseParams(args_vector, src))
    return false;

  int res = 0;
  if (base::StringToInt(region, &res)) {
    switch (res) {
      case 1:
        input_region = gfx::Rect(0,
                                 0,
                                 window_width/2,
                                 window_height/2);
        break;
      case 2:
        input_region = gfx::Rect(window_width/2,
                                 0,
                                 window_width/2,
                                 window_height/2);
        break;
      case 3:
        input_region = gfx::Rect(0,
                                 window_height/2,
                                 window_width/2,
                                 window_height/2);
        break;
      case 4:
        input_region = gfx::Rect(window_width/2,
                                 window_height/2,
                                 window_width/2,
                                 window_height/2);
        break;
      default:
        return false;
    }
    return true;
  }

  return false;
}

}  // namespace

WamDemoService::WamDemoService(const content::MainFunctionParams& parameters)
    : parameters_(parameters) {
  // adding URL
  emulator::EmulatorDataSource* pEmulatorInterface =
      emulator::EmulatorDataSource::GetInstance();
  pEmulatorInterface->AddURLForPolling(emulator::kWam_commandSet, this,
                                       base::ThreadTaskRunnerHandle::Get());
}

WamDemoService::~WamDemoService() {}

void WamDemoService::Launch(const std::string& appid, const std::string& appurl,
                            bool fullscreen, bool frameless) {
  app_runtime::WebAppWindowBase::CreateParams params;
  params.web_contents = nullptr;
  params.type = app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindow;
  params.width = kDefaultWindowWidth;
  params.height = kDefaultWindowHeight;

  if (fullscreen)
    params.show_state = app_runtime::WebAppWindowBase::CreateParams::WindowShowState::kFullscreen;

  params.type = frameless
              ? app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindowFrameless
              : app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindow;

  WebAppWindowImpl* webapp = new WebAppWindowImpl(params, this);
  BlinkView* webpage = new BlinkView(params.width, params.height, this);

  std::string full_appid = wam_demo_app_prefix_ + appid;
  webpage->SetAppId(full_appid);
  webapp->SetWindowProperty("appId", full_appid);

  webpage->LoadUrl(appurl.c_str());
  webapp->AttachWebContents(webpage->GetWebContents());
  webapp->Show();
  appslist_.push_back(WamDemoApplication(appid, appurl, webapp, webpage));

  if (base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kRemoteDebuggingPort)) {
    webpage->SetInspectable(true);
    webpage->EnableInspectablePage();
  }
}

void WamDemoService::DataUpdated(const std::string& url,
                                 const std::string& value) {
  if (url.compare(emulator::kWam_commandSet) != 0)
    return;

  LOG(INFO) << __func__ << "(): Command is delivered: " << value.c_str();
  std::string appid;
  std::string cmd;
  std::string appurl;

  if (!UnpackGeneralParams(value, appid, cmd, appurl))
    return;

  auto app_it = std::find_if(appslist_.begin(), appslist_.end(),
      [&appid](WamDemoApplication& app) { return app.appid_ == appid; });

  if (app_it != appslist_.end()) {
    WebAppWindowImpl* webapp = app_it->win_;
    BlinkView* webpage = app_it->page_;

    if (cmd == command::kChangeUrl) {
      webpage->LoadUrl("about:blank");
      app_it->url_ = appurl;
      webpage->LoadUrl(appurl.c_str());
      EmulatorSendData(response::kAppStarted, appid);
    } else if (cmd == command::kReloadPage) {
      webpage->Reload();
    } else if (cmd == command::kReplaceBaseURL) {
      webpage->ReplaceBaseURL(appurl, app_it->url_);
      // Base URL is replaced now and we need to reload page
      webpage->RunJavaScript("location.reload();");
    } else if (cmd == command::kStopLoading) {
      webpage->StopLoading();
    } else if (cmd == command::kLaunchApp ||
               cmd == command::kLaunchHiddenApp)
      LOG(INFO) << __func__ << "(): Application already started";
    else if (cmd == command::kStopApp)
      webapp->Close();
    else if (cmd == command::kKillApp) {
      int pid = app_it->page_->RenderProcessPid();
      if (pid)
        kill(pid, SIGKILL);
      else
        LOG(INFO) << __func__ << "(): Invalid pid to kill";
    }
    else if (cmd == command::kForegroundApp) {
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
      webapp->Minimize();
      webapp->Restore();
#else
      webapp->Activate();
#endif
    } else if (cmd == command::kUpdateZoom) {
      int zoom;
      if (UnpackInt(value, argument::kZoomFactor, zoom))
        webpage->SetZoomFactor((float)zoom / 100.f);
      else
        LOG(INFO) << __func__ << "(): no valid \'" << argument::kZoomFactor << "\'";
    } else if (cmd == command::kUpdateAppWindow) {
      int x = 0;
      int y = 0;
      int w = 0;
      int h = 0;
      if (UnpackLayoutParams(value, x, y, w, h))
        webapp->SetBounds(x, y, w, h);
      else
        LOG(INFO) << __func__ << "(): Invalid layout parameters";
    } else if (cmd == command::kShowApp) {
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
      webapp->Restore();
#else
      webapp->Show();
#endif
    } else if (cmd == command::kHideApp) {
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
      webapp->Minimize();
#else
      webapp->Hide();
#endif
    } else if (cmd == command::kIsKeyboardVisible) {
        std::string command = response::kKeyboardVisibility;
        command += ':' + std::string(webapp->IsKeyboardVisible() ? "yes" : "no");

        EmulatorSendData(command, appid);
    } else if (cmd == command::kSetWindowProperty) {
      std::string window_property_name;
      std::string window_property_value;
      if (UnpackString(value, argument::kName, window_property_name) &&
          UnpackString(value, argument::kValue, window_property_value))
        webapp->SetWindowProperty(window_property_name, window_property_value);
      else
        LOG(INFO) << __func__ << "(): Invalid window property";
    } else if (cmd == command::kSetBackgroundColor) {
      int r = 0;
      int g = 0;
      int b = 0;
      if (UnpackBackgroundColor(value, r, g, b))
        webpage->SetBackgroundColor(r, g, b, argument::kDefaultAlphaValue);
    } else if (cmd == command::kSetBoardType) {
      std::string board_type;
      if (UnpackString(value, argument::kBoardType, board_type)) {
        LOG(INFO) << __func__ << "(): board_type: " << board_type;
        webpage->SetBoardType(board_type);
      } else
        LOG(INFO) << __func__ << "(): Invalid board type";
    } else if (cmd == command::kLoadInjections) {
      std::vector<std::string> injections;
      if (UnpackInjections(value, injections)) {
        for (const auto& injection : injections)
          webpage->RequestInjectionLoading(injection);
      }
    } else if(cmd == command::kClearInjections) {
      webpage->RequestClearInjections();
    } else if (cmd == command::kDecidePolicyForResponse) {
      webpage->SetDecidePolicyForResponse();
    } else if (cmd == command::kDeleteWebStorages) {
      webpage->DeleteWebStorages(wam_demo_app_prefix_ + appid);
    } else if (cmd == command::kGoBack) {
      webpage->GoBack();
    } else if (cmd == command::kCanGoBack) {
      std::string command = response::kCanGoBackAbility;
      command += ':' + std::string(webpage->CanGoBack() ? "yes" : "no");

      EmulatorSendData(command, appid);
    } else if (cmd == command::kDoNotTrack) {
      bool dnt;
      if (UnpackBool(value, argument::kEnable, dnt))
        webpage->SetDoNotTrack(dnt);
      else
        LOG(INFO) << __func__ << "(): no valid \'" << argument::kEnable << "\'";
    } else if (cmd == command::kIgnoreSSLError) {
      webpage->SetSSLCertErrorPolicy(app_runtime::SSL_CERT_ERROR_POLICY_IGNORE);
    } else if (cmd == command::kResetCompositorPainting) {
      webapp->RecreatedWebContents();
    } else if (cmd == command::kSetHTMLSystemKeyboardEnabled) {
      webpage->SetEnableHtmlSystemKeyboardAttr(true);
    } else if (cmd == command::kSetMediaCodecCapability) {
      std::string media_codec_capability;
      if (UnpackString(value, argument::kMediaCodecCapability, media_codec_capability)) {
        LOG(INFO) << __func__ << "(): media_codec_capability: " << media_codec_capability;
        webpage->SetMediaCodecCapability(media_codec_capability);
      } else
        LOG(INFO) << __func__ << "(): Invalid media codec capability";
    } else if (cmd == command::kAllowUniversalAccessFromFileUrls) {
      webpage->SetAllowUniversalAccessFromFileUrls(true);
    } else if (cmd == command::kResumeDOM) {
      webpage->ResumeWebPageDOM();
    } else if (cmd == command::kSuspendDOM) {
      webpage->SuspendWebPageDOM();
    } else if (cmd == command::kResumeMedia) {
      webpage->ResumeWebPageMedia();
    } else if (cmd == command::kSuspendPainting) {
      webpage->SuspendPaintingAndSetVisibilityHidden();
    } else if (cmd == command::kResumePainting) {
      webpage->ResumePaintingAndSetVisibilityVisible();
    } else if (cmd == command::kSetAcceptLanguage) {
        webpage->SetAcceptLanguages("ko");
        webpage->RunJavaScript("location.reload();");
    } else if (cmd == command::kRunJavaScript) {
      std::string jscode;
      if (UnpackString(value, argument::kJSCode, jscode))
        webpage->RunJavaScript(jscode);
    } else if (cmd == command::kRunJSInAllFrames) {
      webpage->RunJavaScriptInAllFrames("if (location!=parent.location) location=\"https://ebay.com\"");
    } else if (cmd == command::kSuspendMedia) {
      webpage->SuspendWebPageMedia();
    } else if (cmd == command::kSetProxyServer) {
      std::string host;
      std::string port;
      if (UnpackString(value, "server", host) &&
          UnpackString(value, "port", port)) {
        LOG(INFO) << __func__ << " change proxy server to = "  << host << ":" << port;
        webpage->GetProfile()->SetProxyServer(host, port, "", "");
        webpage->LoadUrl("https://yandex.ru/internet");
      }
      else
        LOG(INFO) << __func__ << "(): Invalid params";
    } else if (cmd == command::kExtraWebSocketHeader) {
      std::string header;
      std::string val;
      if (UnpackString(value, "header", header) &&
          UnpackString(value, "value", val)) {
        LOG(INFO) << __func__ << " added extra header = "  << header << ":" << val;
        webpage->GetProfile()->AppendExtraWebSocketHeader(header, val);
        webpage->LoadUrl("https://manytools.org/http-html-text/http-request-headers/");
      }
      else
        LOG(INFO) << __func__ << "(): Invalid params";
    } else if (cmd == command::kGetUserAgent) {
        const std::string user_agent = webpage->UserAgent();
        std::string command = response::kUserAgentIs;
        command += ':' + user_agent;
        EmulatorSendData(command, app_it->appid_);
    } else if (cmd == command::kSetUserAgent) {
      std::string user_agent;
      if (UnpackString(value, argument::kUserAgent, user_agent)) {
        LOG(INFO) << __func__ << "(): user_agent: " << user_agent;
        webpage->SetUserAgent(user_agent);
      } else
        LOG(INFO) << __func__ << "(): Invalid user agent";
    } else if (cmd == command::kGetPid) {
      const std::string pid = base::IntToString(app_it->page_->RenderProcessPid());
      if (!pid.empty()) {
        std::string command = response::kPidRequested;
        command += ':' + pid;
        EmulatorSendData(command, app_it->appid_);
      }
      else
        LOG(INFO) << __func__ << "(): Invalid process ID";
    } else if (cmd == command::kSetFocus) {
      bool set = false;
      if (UnpackBool(value, argument::kSet, set))
        webpage->SetFocus(set);
    } else if (cmd == command::kSetFontFamily) {
      std::string font_family;
      if (UnpackString(value, argument::kFontFamily, font_family))
        webpage->SetStandardFontFamily(font_family);
      else
        LOG(INFO) << __func__ << "(): Invalid font family";
    } else if (cmd == command::kSetWindowState) {
      app_runtime::WidgetState state = app_runtime::WidgetState::UNINITIALIZED;
      if (UnpackWindowState(value, state))
        webapp->SetWindowHostState(state);
    } else if (cmd == command::kSetUseVirtualKeyboard) {
      bool enable = false;
      if (!UnpackBool(value, argument::kEnable, enable))
        LOG(INFO) << __func__ << "(): no valid \'" << argument::kEnable << "\'";
      else
        webapp->SetUseVirtualKeyboard(enable);
    } else if (cmd == command::kResizeWindow) {
      int width = 0, height = 0;
      if (UnpackWindowSize(value, width, height))
        webapp->Resize(width, height);
    } else if (cmd == command::kSetHardwareResolution) {
      int width = 0, height = 0;
      if (UnpackHardwareResolution(value, width, height))
        webpage->SetHardwareResolution(width, height);
    } else if (cmd == command::kXInputActivate) {
      webapp->XInputActivate();
    } else if (cmd == command::kXInputDeactivate) {
      webapp->XInputDeactivate();
    } else if (cmd == command::kXInputInvokeAction) {
      std::string keysym_str;
      if (UnpackString(value, argument::kKeySym, keysym_str)) {
        int keysym;
        if (base::StringToInt(keysym_str, &keysym)) {
          webapp->XInputInvokeAction((uint32_t)keysym);
        }
      }
    } else if (cmd == command::kSetAllowFakeBoldText) {
      bool allow = false;
      if (!UnpackBool(value, argument::kAllow, allow))
        LOG(INFO) << __func__ << "(): no valid \'" << argument::kAllow << "\'";
      else
        webpage->SetAllowFakeBoldText(allow);
    } else if (cmd == command::kSetOpacity) {
      float opacity = 1.f;
      if (UnpackFloat(value, argument::kOpacity, opacity))
        webapp->SetOpacity(opacity);
    } else if (cmd == command::kGetWindowState) {
      const app_runtime::WidgetState new_state = webapp->GetWindowHostState();
      std::string command = std::string(response::kWindowStateRequested) + ':' +
          GetWidgetStateString(new_state);
      EmulatorSendData(command, app_it->appid_);
    } else if (cmd == command::kGetWindowStateAboutToChange) {
      const app_runtime::WidgetState state = webapp->GetWindowHostStateAboutToChange();
      std::string command = std::string(response::kWindowStateAboutToChangeRequested) + ':' +
          GetWidgetStateString(state);
      EmulatorSendData(command, app_it->appid_);
    } else if (cmd == command::kSetScaleFactor) {
      float scale_factor = 1.f;
      if (UnpackFloat(value, argument::kScaleFactor, scale_factor))
        webapp->SetScaleFactor(scale_factor);
    } else if (cmd == command::kAddUserStyleSheet) {
      std::string user_stylesheet;
      if (UnpackString(value, argument::kUserStyleSheet, user_stylesheet)) {
        LOG(INFO) << __func__ << "(): user_stylesheet: " << user_stylesheet;
        webpage->AddUserStyleSheet(user_stylesheet);
      }
      else
        LOG(INFO) << __func__ << "(): Invalid user stylesheet type";
    } else if (cmd == command::kSetCustomCursor) {
      base::FilePath cursor_file;
      bool result = base::PathService::Get(base::DIR_ASSETS, &cursor_file);
      DCHECK(result);
      cursor_file = cursor_file.Append(FILE_PATH_LITERAL("cursor.png"));
      webapp->SetCustomCursor(app_runtime::CustomCursorType::kPath,
                              cursor_file.value().c_str(), 0, 0);
    } else if (cmd == command::kSetVisibilityState) {
      std::string visibility_state;
      if (UnpackString(value, argument::kVisibilityState, visibility_state)) {
        if (visibility_state.compare("Visible") == 0)
          webpage->SetVisibilityState(
              app_runtime::WebPageVisibilityState::WebPageVisibilityStateVisible);
        else if (visibility_state.compare("Hidden") == 0)
          webpage->SetVisibilityState(
              app_runtime::WebPageVisibilityState::WebPageVisibilityStateHidden);
        else if (visibility_state.compare("Launching") == 0)
          webpage->SetVisibilityState(
              app_runtime::WebPageVisibilityState::WebPageVisibilityStateLaunching);
      }
      else
        LOG(INFO) << __func__ << "(): Invalid visibility state";
    } else if (cmd == command::kSetGroupKeyMask) {
      int key_mask = 0;
      if (UnpackInt(value, argument::kKeyMask, key_mask))
        webapp->SetGroupKeyMask(static_cast<app_runtime::KeyMask>(key_mask));
    } else if (cmd == command::kSetKeyMask) {
      int key_mask = 0;
      bool set = false;
      if (UnpackInt(value, argument::kKeyMask, key_mask) &&
          UnpackBool(value, argument::kSet, set))
        webapp->SetKeyMask(static_cast<app_runtime::KeyMask>(key_mask), set);
    } else if (cmd == command::kEnableInspectablePage) {
      webpage->EnableInspectablePage();
    } else if (cmd == command::kSetInspectable) {
      bool enable;
      if (UnpackBool(value, argument::kEnable, enable))
        webpage->SetInspectable(enable);
      else
        LOG(INFO) << __func__ << "(): no valid \'" << argument::kEnable << "\'";
    } else if (cmd == command::kGetDevToolsEndpoint) {
      std::string command = response::kDevToolsEndpoint;
      char* ip = ::getenv("NEVA_EMULATOR_SERVER_ADDRESS");
      int port = emulator::kEmulatorDefaultPort;
      command += ':';
      command.append(util::get_my_ip_to(ip, port));
      command += ',';
      command.append(base::IntToString(webpage->DevToolsPort()));

      EmulatorSendData(command, appid);
    } else if (cmd == command::kSetInputRegion) {
      gfx::Rect input_region;
      int width = webapp->GetWidth();
      int height = webapp->GetHeight();
      if (UnpackInputRegion(value, argument::kInputRegion, input_region, width, height)) {
        std::vector<gfx::Rect> region;
        region.push_back(input_region);
        webapp->SetInputRegion(region);
      }
    } else if (cmd == command::kSetMediaCapturePermission) {
      webpage->SetMediaCapturePermission();
    } else if (cmd == command::kClearMediaCapturePermission) {
      webpage->ClearMediaCapturePermission();
    }
    else
      LOG(INFO) << __func__ << "(): Command \'" << cmd
                << "\' is not supported in current mode";
  } else {
    if (cmd == command::kLaunchApp) {
      LaunchApp(value, appid, appurl, true);
      EmulatorSendData(response::kAppStarted, appid);
    } else if (cmd == command::kLaunchHiddenApp) {
      LaunchApp(value, appid, appurl, false);
      EmulatorSendData(response::kAppStarted, appid);
    } else
      LOG(INFO) << __func__ << "(): Command \'" << cmd
                << "\' is not supported in current mode";
  }
}

void WamDemoService::OnWindowClosing(WebAppWindowImpl* window) {
  auto app_it = std::find_if(appslist_.begin(), appslist_.end(),
      [window](WamDemoApplication& app) { return app.win_ == window; });

      // Do not remove red underline on wam_emulator_page.html
      // if render process gone before
  if (app_it != appslist_.end()) {
    if (!app_it->render_gone_)
      EmulatorSendData(response::kAppClosed, app_it->appid_);
    // TODO(stanislav.pikulik) Not sure that we need to delete it manually.
    delete app_it->win_;
    delete app_it->page_;
    appslist_.erase(app_it);
  }
}

void WamDemoService::OnTitleChanged(app_runtime::WebViewBase* view,
                                const std::string& title) {
  auto app_it = std::find_if(appslist_.begin(), appslist_.end(),
      [view](WamDemoApplication& app) { return app.page_ == view; });

  if (app_it != appslist_.end())
    app_it->win_->SetWindowTitle(title);
}

void WamDemoService::EmulatorSendData(const std::string& command,
                                      const std::string& id) {
  emulator::RequestArgs args_vector = {
    {"arg1", &command},
    {"arg2", &id}
  };

  std::string params =
      emulator::EmulatorDataSource::PrepareRequestParams(args_vector);
  emulator::EmulatorDataSource::SetExpectationAsync(
      emulator::kWam_callFunc, params);
}

void WamDemoService::BrowserControlCommandNotify(
    const std::string& name,
    const std::vector<std::string>& args) {
  emulator::RequestArgs args_vector = {
    {"name", &name},
  };

  std::vector<std::string> names;
  names.reserve(args.size());
  for (std::size_t i = 0; i < args.size(); ++i) {
    std::stringstream name_builder;
    name_builder << "arg" << (i+1);
    names.push_back(name_builder.str());
    args_vector.push_back({names.back().c_str(), &args[i]});
  }

  const std::string params =
      emulator::EmulatorDataSource::PrepareRequestParams(args_vector);
  emulator::EmulatorDataSource::SetExpectationAsync(
      emulator::kBrowserControl_sendCommand, params);
}

void WamDemoService::BrowserControlFunctionNotify(
    const std::string& name,
    const std::vector<std::string>& args,
    const std::string& result) {
  emulator::RequestArgs args_vector = {
    {"name", &name},
    {"result", &result},
  };

  std::vector<std::string> names;
  names.reserve(args.size());
  for (std::size_t i = 0; i < args.size(); ++i) {
    std::stringstream name_builder;
    name_builder << "arg" << (i+1);
    names.push_back(name_builder.str());
    args_vector.push_back({names.back().c_str(), &args[i]});
  }

  const std::string params =
      emulator::EmulatorDataSource::PrepareRequestParams(args_vector);
  emulator::EmulatorDataSource::SetExpectationAsync(
      emulator::kBrowserControl_callFunction, params);
}

void WamDemoService::OnRenderProcessGone(app_runtime::WebViewBase* view) {
  SendCommandWithView(view, response::kProcessGone);

  auto app_it = std::find_if(appslist_.begin(), appslist_.end(),
      [&view](WamDemoApplication& app) { return app.page_ == view; });

  if (app_it != appslist_.end()) {
    // Set the flag which indicates that app was not closed normally.
    // It is needed to keep red underline on wam_emulator_page.html.
    app_it->render_gone_ = true;
    app_it->win_->Close();
  }
}

void WamDemoService::OnRenderProcessCreated(app_runtime::WebViewBase* view) {
  SendCommandWithView(view, response::kAppStarted);
}

void WamDemoService::OnDocumentLoadFinished(app_runtime::WebViewBase* view) {
  auto app_it = std::find_if(appslist_.begin(), appslist_.end(),
      [view](WamDemoApplication& app) { return app.page_ == view; });

  if (app_it != appslist_.end()) {
    const std::string zoom = base::IntToString(app_it->page_->GetZoomFactor() * 100);
    if (!zoom.empty()) {
      std::string command = response::kZoomUpdated;
      command += ':' + zoom;
      EmulatorSendData(command, app_it->appid_);
    }

    const std::string pid = base::IntToString(app_it->page_->RenderProcessPid());
    if (!pid.empty()) {
      std::string command = response::kPidUpdated;
      command += ':' + pid;
      EmulatorSendData(command, app_it->appid_);
    }

    EmulatorSendData(response::kLoadFinished, app_it->appid_);
  }
}

void WamDemoService::OnLoadFailed(app_runtime::WebViewBase* view) {
  SendCommandWithView(view, response::kLoadFailed);
}

bool WamDemoService::SendCommandWithView(app_runtime::WebViewBase* view,
                                         const std::string& cmd) {
  for (const WamDemoApplication& app : appslist_) {
    if (app.page_ == view) {
      EmulatorSendData(cmd.c_str(), app.appid_);
      return true;
    }
  }
  return false;
}

void WamDemoService::OnBrowserControlCommand(
    const std::string& name, const std::vector<std::string>& args) {
  BrowserControlCommandNotify(name, args);
}

std::string WamDemoService::OnBrowserControlFunction(
    const std::string& name, const std::vector<std::string>& args) {
  std::string result;
  std::stringstream ss;
  ss << args.size();
  ss >> result;
  BrowserControlFunctionNotify(name, args, result);
  return result;
}

void WamDemoService::LaunchApp(const std::string& value,
                               const std::string& appid,
                               const std::string& appurl,
                               bool shown) {
  app_runtime::WebAppWindowBase::CreateParams params;
  params.web_contents = nullptr;

  bool is_fullscreen = false;
  if (!UnpackBool(value, argument::kFullScreen, is_fullscreen))
    LOG(INFO) << __func__ << "(): no valid \'" << argument::kFullScreen << "\'";

  if (is_fullscreen)
    params.show_state = app_runtime::WebAppWindowBase::CreateParams::WindowShowState::kFullscreen;
  else {
    UnpackLayoutParams(
        value, params.pos_x, params.pos_y, params.width, params.height);
  }

  bool is_frameless = false;
  if (!UnpackBool(value, argument::kFramelessWindow, is_frameless)) {
    LOG(INFO) << __func__ << "(): no valid \'"
              << argument::kFramelessWindow << "\'";
  }

  params.type = is_frameless
              ? app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindowFrameless
              : app_runtime::WebAppWindowBase::CreateParams::WidgetType::kWindow;

  WebAppWindowImpl* webapp = new WebAppWindowImpl(params, this);
  BlinkView* webpage = new BlinkView(params.width, params.height, this);

  int vpw = 0;
  int vph = 0;
  if (UnpackViewportParams(value, vpw, vph))
    webpage->SetViewportSize(vpw, vph);
  else
    LOG(INFO) << __func__ << "(): Invalid viewport parameters";

  std::vector<std::string> injections;
  if (UnpackInjections(value, injections)) {
    for (const auto& injection : injections)
      webpage->RequestInjectionLoading(injection);
  }

  std::string full_appid = wam_demo_app_prefix_ + appid;
  webpage->SetAppId(full_appid);
  webapp->SetWindowProperty("appId", full_appid);
  webpage->SetSecurityOrigin(full_appid);

  webpage->LoadUrl(appurl.c_str());
  webapp->AttachWebContents(webpage->GetWebContents());

  if (shown)
    webapp->Show();
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  else
    webapp->Minimize();
#endif


  if (base::CommandLine::ForCurrentProcess()->
        GetSwitchValueASCII(switches::kTestType) == "webdriver")
    webpage->EnableInspectablePage();

  appslist_.push_back(WamDemoApplication(appid, appurl, webapp, webpage));
}

}  // namespace wam_demo
