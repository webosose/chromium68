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

#include "webos/common/webos_watchdog.h"

#include <signal.h>

#include "base/command_line.h"
#include "base/logging.h"
#include "content/public/common/content_switches.h"

namespace webos {

static const int kDefaultWatchdogTimeout = 100;
static const int kDefaultWatchdogPeriod = 20;
static const int kWatchdogCleanupPeriod = 10;

WebOSWatchdog::WebOSWatchdog()
    : period_(kDefaultWatchdogPeriod),
      timeout_(kDefaultWatchdogTimeout),
      watching_tid_(0) {
}

WebOSWatchdog::~WebOSWatchdog() {
  if (watchdog_thread_) {
    watchdog_thread_->Disarm();
    watchdog_thread_->Cleanup();
  }
}

void WebOSWatchdog::StartWatchdog() {
  watchdog_thread_.reset(
      new WatchdogThread(base::TimeDelta::FromSeconds(timeout_), this));
}

void WebOSWatchdog::Arm() {
  watchdog_thread_->Arm();
}

WebOSWatchdog::WatchdogThread::WatchdogThread(const base::TimeDelta& duration,
                                              WebOSWatchdog* watchdog)
    : base::Watchdog(duration, "WebOSWatchdog", true), watchdog_(watchdog) {
}

void WebOSWatchdog::WatchdogThread::Alarm() {
// kill process
  RAW_PMLOG_INFO("WatchdogThread",
                 "Stuck detected in thread %d in process %d! Kill %d process",
                 watchdog_->WatchingThreadTid(),
                 getpid(),
                 getpid());
  kill(getpid(), SIGABRT);
}

}  // namespace webos
