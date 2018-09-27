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

#ifndef COMPONENTS_WATCHDOG_WATCHDOG_H_
#define COMPONENTS_WATCHDOG_WATCHDOG_H_

#include <memory>

#include <sys/syscall.h>
#include <sys/types.h>

#include "base/threading/watchdog.h"
#include "base/time/time.h"

namespace watchdog {

class Watchdog {
 public:
  Watchdog();
  virtual ~Watchdog();

  void StartWatchdog();
  void Arm();

  void SetPeriod(int period) { period_ = period; }
  int GetPeriod() { return period_; }

  void SetTimeout(int timeout) { timeout_ = timeout; }

  void SetWatchingThreadTid(pid_t tid) { watching_tid_ = tid; }
  int GetWatchingThreadTid() { return watching_tid_; }

 private:
  class WatchdogThread : public base::Watchdog {
   public:
    WatchdogThread(const base::TimeDelta& duration, watchdog::Watchdog* watchdog);

    void Alarm() override;

   private:
    watchdog::Watchdog* watchdog_;
  };

  std::unique_ptr<base::Watchdog> watchdog_thread_;
  int period_;
  int timeout_;
  pid_t watching_tid_;
};

}  // namespace watchdog

#endif /* COMPONENTS_WATCHDOG_WATCHDOG_H_ */
