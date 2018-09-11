// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MANAGER_H_
#define BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MANAGER_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <set>
#include <vector>

#include "base/atomicops.h"
#include "base/containers/hash_tables.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/singleton.h"
#include "base/synchronization/lock.h"
#include "base/timer/timer.h"
#include "base/trace_event/trace_event.h"

namespace base {

class SingleThreadTaskRunner;
class Thread;

namespace trace_event {
namespace neva {

class MemoryTraceManagerDelegate;
class MemoryTraceProvider;

using MemoryTraceCallback = Callback<void(bool success)>;

class BASE_EXPORT MemoryTraceManager {
 public:
  static MemoryTraceManager* GetInstance();

  bool Initialize(MemoryTraceManagerDelegate* delegate,
                  bool is_browser_process);

  void RegisterTraceProvider(MemoryTraceProvider* mtp,
                            const char* name,
                            scoped_refptr<SingleThreadTaskRunner> task_runner);

  void UnregisterTraceProvider(MemoryTraceProvider* mtp);

  void RequestGlobalTrace(const MemoryTraceCallback& callback);

  // Same as above (still asynchronous), but without callback.
  void RequestGlobalTrace();

  FILE* GetTraceFile() { return trace_file_; }

  bool IsTraceLogCSV() { return is_trace_log_csv_; }

  bool GetUseMegaBytes() { return use_mega_bytes_; }

 private:
  friend std::default_delete<MemoryTraceManager>;  // For the testing instance.
  friend struct DefaultSingletonTraits<MemoryTraceManager>;
  friend class MemoryTraceManagerDelegate;

  struct MemoryTraceProviderInfo
      : public RefCountedThreadSafe<MemoryTraceProviderInfo> {
    // Define a total order based on the |task_runner| affinity, so that MTPs
    // belonging to the same SequencedTaskRunner are adjacent in the set.
    struct Comparator {
      bool operator()(const scoped_refptr<MemoryTraceProviderInfo>& a,
                      const scoped_refptr<MemoryTraceProviderInfo>& b) const;
    };
    using OrderedSet =
        std::set<scoped_refptr<MemoryTraceProviderInfo>, Comparator>;

    MemoryTraceProviderInfo(MemoryTraceProvider* trace_provider,
                           const char* name,
                           scoped_refptr<SequencedTaskRunner> task_runner);

    MemoryTraceProvider* const trace_provider;

    // Human readable name, for debugging and testing. Not necessarily unique.
    const char* const name;

    // The task runner affinity. Can be nullptr, in which case the trace provider
    // will be invoked on |trace_thread_|.
    const scoped_refptr<SequencedTaskRunner> task_runner;

    // Flagged either by the auto-disable logic or during unregistration.
    bool disabled;

   private:
    friend class base::RefCountedThreadSafe<MemoryTraceProviderInfo>;
    ~MemoryTraceProviderInfo();

    DISALLOW_COPY_AND_ASSIGN(MemoryTraceProviderInfo);
  };

  // Sets up periodic memory trace timers to start global trace requests based on
  // the trace triggers from trace config.
  class BASE_EXPORT PeriodicGlobalTraceTimer {
   public:
    PeriodicGlobalTraceTimer();
    ~PeriodicGlobalTraceTimer();

    void Start(uint32_t timer_period_secs);
    void Stop();

    bool IsRunning();

   private:
    // Periodically called by the timer.
    void RequestPeriodicGlobalTrace();

    RepeatingTimer timer_;

    DISALLOW_COPY_AND_ASSIGN(PeriodicGlobalTraceTimer);
  };

  static const char* const kSystemAllocatorPoolName;

  MemoryTraceManager();
  ~MemoryTraceManager();

  void InvokeOnMemoryTrace();

  void RegisterTraceProviderInternal(
      MemoryTraceProvider* mtp,
      const char* name,
      scoped_refptr<SequencedTaskRunner> task_runner);

  void UnregisterTraceProviderInternal(MemoryTraceProvider* mtp);

  bool SetupCommandLineOptions(bool is_browser_process);

  MemoryTraceProviderInfo::OrderedSet trace_providers_;

  MemoryTraceManagerDelegate* delegate_;  // Not owned.

  // Protects from concurrent accesses to the |trace_providers_*| and |delegate_|
  // to guard against disabling logging while tracing on another thread.
  Lock lock_;

  // For time-triggered periodic traces.
  PeriodicGlobalTraceTimer periodic_trace_timer_;

  // Timer interval in seconds when --trace-memory-interval is given.
  // Default tracing interval is 5 seconds.
  uint32_t timer_period_secs_ = 5;

  // elapsed time = global_trace_count_ * timer_period_secs_
  uint32_t global_trace_count_ = 0;

  // Trace output file.
  FILE* trace_file_;

  // If it's true, output log becomes CSV format.
  bool is_trace_log_csv_ = false;

  // Whether to show memory unit in MB.
  // It can be changed by --trace-memory-byte-unit=mb
  bool use_mega_bytes_ = false;

  DISALLOW_COPY_AND_ASSIGN(MemoryTraceManager);
};

// The delegate is supposed to be long lived (read: a Singleton) and thread
// safe (i.e. should expect calls from any thread and handle thread hopping).
class BASE_EXPORT MemoryTraceManagerDelegate {
 public:
  bool Initialize(bool is_browser_process);

  void RequestGlobalMemoryTrace(const MemoryTraceCallback& callback);

  MemoryTraceManagerDelegate() {}
  ~MemoryTraceManagerDelegate() {}

 private:
  DISALLOW_COPY_AND_ASSIGN(MemoryTraceManagerDelegate);
};

}  // namespace neva
}  // namespace trace_event
}  // namespace base

#endif  // BASE_TRACE_EVENT_NEVA_MEMORY_TRACE_MANAGER_H_
