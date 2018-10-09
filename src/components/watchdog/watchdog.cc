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

#include <signal.h>

#include "components/watchdog/watchdog.h"

#include "base/command_line.h"
#include "base/logging_pmlog.h"

namespace watchdog {

static const int kDefaultWatchdogTimeout = 100;
static const int kDefaultWatchdogPeriod = 20;
static const int kWatchdogCleanupPeriod = 10;

Watchdog::Watchdog()
    : period_(kDefaultWatchdogPeriod),
      timeout_(kDefaultWatchdogTimeout),
      watching_tid_(0) {
}

Watchdog::~Watchdog() {
  if (watchdog_thread_) {
    watchdog_thread_->Disarm();
    watchdog_thread_->Cleanup();
  }
}

void Watchdog::StartWatchdog() {
  watchdog_thread_.reset(
      new WatchdogThread(base::TimeDelta::FromSeconds(timeout_), this));
}

void Watchdog::Arm() {
  watchdog_thread_->Arm();
}

Watchdog::WatchdogThread::WatchdogThread(const base::TimeDelta& duration,
                                              watchdog::Watchdog* watchdog)
    : base::Watchdog(duration, "Watchdog", true), watchdog_(watchdog) {
}

void Watchdog::WatchdogThread::Alarm() {
  // kill process
  PMLOG_INFO(Raw,
      "WatchdogThread",
      "Detected stuck thread %d in process %d! Killing process with SIGABRT",
      watchdog_->GetWatchingThreadTid(), getpid(), getpid());
  kill(getpid(), SIGABRT);
}

}  // namespace watchdog
