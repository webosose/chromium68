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

#include "neva/app_runtime/browser/app_runtime_browser_main_parts.h"

#include "base/command_line.h"
#include "base/logging.h"
#include "base/run_loop.h"
#include "base/strings/string_number_conversions.h"
#include "components/os_crypt/key_storage_config_linux.h"
#include "components/os_crypt/os_crypt.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/result_codes.h"
#include "net/base/network_change_notifier_factory_neva.h"
#include "neva/app_runtime/browser/app_runtime_browser_context_adapter.h"
#include "neva/app_runtime/browser/app_runtime_browser_main_extra_parts.h"
#include "neva/app_runtime/browser/app_runtime_devtools_manager_delegate.h"
#include "neva/app_runtime/browser/url_request_context_factory.h"
#include "ui/base/material_design/material_design_controller.h"

#if defined(ENABLE_PLUGINS)
#include "content/public/browser/plugin_service.h"
#endif

#if defined(USE_AURA)
#include "ui/aura/env.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/display/screen.h"
#include "ui/views/widget/desktop_aura/desktop_screen.h"
#endif

#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
#include "ozone/ui/webui/ozone_webui.h"
#endif

#if defined(OS_WEBOS)
#include "content/public/browser/luna_service_delegate.h"
#include "neva/app_runtime/browser/webos/webos_luna_service.h"
#endif

namespace app_runtime {

AppRuntimeBrowserMainParts::AppRuntimeBrowserMainParts(
    URLRequestContextFactory* url_request_context_factory,
    content::BrowserContext*)
    : BrowserMainParts(),
      url_request_context_factory_(url_request_context_factory) {}

AppRuntimeBrowserMainParts::~AppRuntimeBrowserMainParts() {}

void AppRuntimeBrowserMainParts::AddParts(
    AppRuntimeBrowserMainExtraParts* parts) {
  app_runtime_extra_parts_.push_back(parts);
}

int AppRuntimeBrowserMainParts::DevToolsPort() const {
  return AppRuntimeDevToolsManagerDelegate::GetHttpHandlerPort();
}

void AppRuntimeBrowserMainParts::EnableDevTools() {
  if (dev_tools_enabled_)
    return;

  AppRuntimeDevToolsManagerDelegate::StartHttpHandler(
    browser_context_adapter_->GetBrowserContext());
  dev_tools_enabled_ = true;
}

void AppRuntimeBrowserMainParts::DisableDevTools() {
  if (!dev_tools_enabled_)
    return;

  AppRuntimeDevToolsManagerDelegate::StopHttpHandler();
  dev_tools_enabled_ = false;
}

int AppRuntimeBrowserMainParts::PreEarlyInitialization() {
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  views::LinuxUI::SetInstance(BuildWebUI());
#else
  ui::InitializeInputMethodForTesting();
#endif
  return service_manager::RESULT_CODE_NORMAL_EXIT;
}

void AppRuntimeBrowserMainParts::ToolkitInitialized() {
#if defined(USE_OZONE) && defined(OZONE_PLATFORM_WAYLAND_EXTERNAL)
  views::LinuxUI::instance()->Initialize();
#endif
}

void AppRuntimeBrowserMainParts::PreMainMessageLoopRun() {
#if defined(OS_WEBOS)
  neva::WebOSLunaService::GetInstance()->Initialize();
  content::SetLunaServiceDelegate(neva::WebOSLunaService::GetInstance());
#endif
  url_request_context_factory_->InitializeOnUIThread();

  browser_context_adapter_.reset(
      new BrowserContextAdapter("Default", url_request_context_factory_));

#if defined(ENABLE_PLUGINS)
  plugin_service_filter_.reset(new AppRuntimePluginServiceFilter);
  content::PluginService::GetInstance()->SetFilter(
      plugin_service_filter_.get());
#endif

#if defined(USE_AURA)
  if (!display::Screen::GetScreen())
    display::Screen::SetScreenInstance(views::CreateDesktopScreen());

  aura::Env::GetInstance();
#endif

  ui::MaterialDesignController::Initialize();
  CreateOSCryptConfig();

  // Replace the default NetworkChangeNotifierFactory with browser
  // implementation. This must be done before BrowserMainLoop calls
  // net::NetworkChangeNotifier::Create() in MainMessageLoopStart().
  net::NetworkChangeNotifier::SetFactory(
      new neva::NetworkChangeNotifierFactoryNeva());

  for (auto* extra_part : app_runtime_extra_parts_)
    extra_part->PreMainMessageLoopRun();

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kEnableWatchdog)) {
    ui_watchdog_.reset(new watchdog::Watchdog());
    io_watchdog_.reset(new watchdog::Watchdog());

    std::string env_timeout =
        command_line.GetSwitchValueASCII(switches::kWatchdogBrowserTimeout);
    if (!env_timeout.empty()) {
      int timeout;
      base::StringToInt(env_timeout, &timeout);
      ui_watchdog_->SetTimeout(timeout);
      io_watchdog_->SetTimeout(timeout);
    }

    std::string env_period =
        command_line.GetSwitchValueASCII(switches::kWatchdogBrowserPeriod);
    if (!env_period.empty()) {
      int period;
      base::StringToInt(env_period, &period);
      ui_watchdog_->SetPeriod(period);
      io_watchdog_->SetPeriod(period);
    }

    ui_watchdog_->StartWatchdog();
    io_watchdog_->StartWatchdog();

    content::BrowserThread::PostTask(
        content::BrowserThread::UI, FROM_HERE,
        base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog,
                   base::Unretained(this), content::BrowserThread::UI,
                   ui_watchdog_.get()));

    content::BrowserThread::PostTask(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog,
                   base::Unretained(this), content::BrowserThread::IO,
                   io_watchdog_.get()));
  }
}

void AppRuntimeBrowserMainParts::ArmWatchdog(content::BrowserThread::ID thread,
                                             watchdog::Watchdog* watchdog) {
  watchdog->Arm();
  if (!watchdog->GetWatchingThreadTid())
    watchdog->SetWatchingThreadTid((pid_t)syscall(SYS_gettid));

  content::BrowserThread::PostDelayedTask(
      thread, FROM_HERE,
      base::Bind(&AppRuntimeBrowserMainParts::ArmWatchdog, base::Unretained(this),
                 thread, watchdog),
      base::TimeDelta::FromSeconds(watchdog->GetPeriod()));
}

bool AppRuntimeBrowserMainParts::MainMessageLoopRun(int* result_code) {
  base::RunLoop run_loop;
  run_loop.Run();
  return true;
}

void AppRuntimeBrowserMainParts::PostMainMessageLoopRun() {
  DisableDevTools();
}

void AppRuntimeBrowserMainParts::CreateOSCryptConfig() {
  std::unique_ptr<os_crypt::Config> config(new os_crypt::Config());
  // Forward to os_crypt the flag to use a specific password store.
  config->store = "";
  // Forward the product name
  config->product_name = "";
  // OSCrypt may target keyring, which requires calls from the main thread.
  config->main_thread_runner = content::BrowserThread::GetTaskRunnerForThread(
      content::BrowserThread::UI);
  // OSCrypt can be disabled in a special settings file.
  config->should_use_preference = false;
  config->user_data_path = base::FilePath();
  OSCrypt::SetConfig(std::move(config));
}

}  // namespace app_runtime
