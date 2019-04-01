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

#include <pthread.h>
#include <sys/types.h>
#include <memory>

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

  void SetCurrentThreadInfo();
  bool HasThreadInfo() const;

  pthread_t GetWatchingPthreadId() const { return watching_pthread_id_; }
  pid_t GetWatchingThreadTid() const { return watching_thread_tid_; }

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

  pthread_t watching_pthread_id_;
  pid_t watching_thread_tid_;
};

}  // namespace watchdog

#endif /* COMPONENTS_WATCHDOG_WATCHDOG_H_ */
