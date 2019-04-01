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

#include "components/watchdog/watchdog.h"

#include <execinfo.h>
#include <signal.h>
#include <sys/syscall.h>
#include <fstream>
#include <sstream>
#include <string>

#include "base/command_line.h"
#include "base/logging_pmlog.h"

namespace watchdog {

static const int kDefaultWatchdogTimeout = 100;
static const int kDefaultWatchdogPeriod = 20;
static const int kWatchdogCleanupPeriod = 10;

__attribute__((noinline)) static void _callstack_signal_handler(int signal,
                                                                siginfo_t* info,
                                                                void* secret) {
  if (signal != SIGUSR2)
    return;

  PMLOG_INFO(Raw, "Watchdog_SigHandler", "Stuck handling at thread : %d",
             (long int)syscall(SYS_gettid));

  void* callstack[128];
  const int nMaxFrames = sizeof(callstack) / sizeof(callstack[0]);
  char buf[1024];
  int nFrames = backtrace(callstack, nMaxFrames);
  char** symbols = backtrace_symbols(callstack, nFrames);

  for (int i = 1; i < nFrames; i++) {
    snprintf(buf, sizeof(buf), "%s", symbols[i]);
    PMLOG_INFO(Raw, "Watchdog_Callstack", "%s", buf);
  }

  free(symbols);
  if (nFrames == nMaxFrames)
    PMLOG_INFO(Raw, "Watchdog_Callstack", "[truncated]");

  kill(getpid(), SIGABRT);
}

Watchdog::Watchdog()
    : period_(kDefaultWatchdogPeriod),
      timeout_(kDefaultWatchdogTimeout),
      watching_pthread_id_(0),
      watching_thread_tid_(0) {}

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
  PMLOG_INFO(
      Raw, "WatchdogThread",
      "Detected stuck thread %d in process %d! Killing process with SIGABRT",
      (long int)watchdog_->GetWatchingThreadTid(), getpid());

  struct sigaction sa;
  sigfillset(&sa.sa_mask);
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = _callstack_signal_handler;
  sigaction(SIGUSR2, &sa, NULL);

  if (pthread_kill(watchdog_->GetWatchingPthreadId(), SIGUSR2) != 0) {
    PMLOG_INFO(Raw, "WatchdogThread",
               "Cannot send signal!! Process cannot be recovered!!");
  }
}

void Watchdog::SetCurrentThreadInfo() {
  watching_pthread_id_ = pthread_self();
  watching_thread_tid_ = syscall(SYS_gettid);
}

bool Watchdog::HasThreadInfo() const {
  return watching_pthread_id_ && watching_thread_tid_;
}

}  // namespace watchdog
