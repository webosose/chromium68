// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/trace_event/neva/memory_trace/memory_trace_manager.h"

#include <algorithm>
#include <utility>

#include "base/atomic_sequence_num.h"
#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/debug/stack_trace.h"
#include "base/files/file_util.h"
#include "base/memory/ptr_util.h"
#include "base/neva/base_switches.h"
#include "base/strings/string_number_conversions.h"
#include "base/threading/thread.h"
#include "base/threading/thread_task_runner_handle.h"
#include "base/trace_event/neva/memory_trace/malloc_trace_provider.h"
#include "base/trace_event/neva/memory_trace/memory_trace_provider.h"
#include "base/trace_event/neva/memory_trace/system_trace_provider.h"

namespace base {
namespace trace_event {
namespace neva {

namespace {

// Callback wrapper to hook upon the completion of RequestGlobalTrace() and
// inject trace markers.
void OnGlobalTraceDone(MemoryTraceCallback wrapped_callback, bool success) {
  if (!wrapped_callback.is_null()) {
    wrapped_callback.Run(success);
    wrapped_callback.Reset();
  }
}

}  // namespace

// static
MemoryTraceManager* MemoryTraceManager::GetInstance() {
  return Singleton<MemoryTraceManager,
                   LeakySingletonTraits<MemoryTraceManager>>::get();
}

MemoryTraceManager::MemoryTraceManager()
    : delegate_(nullptr), trace_file_(nullptr) {
}

MemoryTraceManager::~MemoryTraceManager() {
  // Disable periodic traces
  periodic_trace_timer_.Stop();

  if (trace_file_ != stderr)
    CloseFile(trace_file_);
}

bool MemoryTraceManager::Initialize(MemoryTraceManagerDelegate* delegate,
                                    bool is_browser_process) {
  {
    AutoLock lock(lock_);
    DCHECK(delegate);
    DCHECK(!delegate_);
    delegate_ = delegate;
    if (!SetupCommandLineOptions(is_browser_process))
      return false;
  }

  RegisterTraceProvider(SystemTraceProvider::GetInstance(), "System", nullptr);

  RegisterTraceProvider(MallocTraceProvider::GetInstance(), "Malloc", nullptr);

  if (is_trace_log_csv_) {
    fprintf(trace_file_, "pid, elapsed");
    // Iterate in a reverse order, then print header of each
    // MemoryTraceProvider.
    for (auto it = trace_providers_.rbegin();
              it != trace_providers_.rend();
              ++it) {
      auto& mtpinfo = *it;
      MemoryTraceProvider* mtp = mtpinfo->trace_provider;
      std::string header = mtp->GetCSVHeader().c_str();

      if (!header.empty())
        fprintf(trace_file_, ", %s", header.c_str());
    }
    fprintf(trace_file_, "\n");
  }

  // Enable periodic traces
  periodic_trace_timer_.Start(timer_period_secs_);

  return true;
}

void MemoryTraceManager::RegisterTraceProvider(
    MemoryTraceProvider* mtp,
    const char* name,
    scoped_refptr<SingleThreadTaskRunner> task_runner) {
  RegisterTraceProviderInternal(mtp, name, std::move(task_runner));
}

void MemoryTraceManager::RegisterTraceProviderInternal(
    MemoryTraceProvider* mtp,
    const char* name,
    scoped_refptr<SequencedTaskRunner> task_runner) {
  scoped_refptr<MemoryTraceProviderInfo> mtpinfo =
      new MemoryTraceProviderInfo(mtp, name, std::move(task_runner));
  {
    AutoLock lock(lock_);
    bool already_registered = !trace_providers_.insert(mtpinfo).second;
    // This actually happens in some tests which don't have a clean tear-down
    // path for RenderThreadImpl::Init().
    if (already_registered)
      return;
  }
}

void MemoryTraceManager::UnregisterTraceProvider(MemoryTraceProvider* mtp) {
  UnregisterTraceProviderInternal(mtp);
}

void MemoryTraceManager::UnregisterTraceProviderInternal(
    MemoryTraceProvider* mtp) {
  AutoLock lock(lock_);

  auto mtp_iter = trace_providers_.begin();
  for (; mtp_iter != trace_providers_.end(); ++mtp_iter) {
    if ((*mtp_iter)->trace_provider == mtp)
      break;
  }

  if (mtp_iter == trace_providers_.end())
    return;  // Not registered / already unregistered.

  (*mtp_iter)->disabled = true;
  trace_providers_.erase(mtp_iter);
}

void MemoryTraceManager::RequestGlobalTrace(
    const MemoryTraceCallback& callback) {
  // Creates an async event to keep track of the global trace evolution.
  // The |wrapped_callback| will generate the ASYNC_END event and then invoke
  // the real |callback| provided by the caller.
  MemoryTraceCallback wrapped_callback = Bind(&OnGlobalTraceDone, callback);

  MemoryTraceManagerDelegate* delegate;
  {
    AutoLock lock(lock_);
    delegate = delegate_;
  }

  delegate->RequestGlobalMemoryTrace(wrapped_callback);
}

void MemoryTraceManager::RequestGlobalTrace() {
  RequestGlobalTrace(MemoryTraceCallback());
}

void MemoryTraceManager::InvokeOnMemoryTrace() {
  const char* print_fmt;
  uint64_t elapsed = timer_period_secs_ * ++global_trace_count_;

  DCHECK(trace_file_);
  if (!is_trace_log_csv_) {
    print_fmt = "=== Memory Trace Result (pid: %d, elapsed: %lu secs) ===\n";
  } else {
    print_fmt = "%d, %lu";
  }
  fprintf(trace_file_, print_fmt, getpid(), elapsed);

  // Iterate in a reverse order, then invoke OnMemoryTrace().
  for (auto it = trace_providers_.rbegin();
            it != trace_providers_.rend();
            ++it) {
    if (is_trace_log_csv_)
      fprintf(trace_file_, ", ");

    auto& mtpinfo = *it;
    mtpinfo->trace_provider->OnMemoryTrace();
  }

  fprintf(trace_file_, "\n");
  fflush(trace_file_);
}

bool MemoryTraceManager::SetupCommandLineOptions(bool is_browser_process) {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();

  if (is_browser_process) {
    if (!command_line.HasSwitch(switches::kTraceMemoryBrowser))
      return false;
  } else {
    if (!command_line.HasSwitch(switches::kTraceMemoryRenderer))
      return false;
  }

  if (command_line.HasSwitch(switches::kTraceMemoryInterval)) {
    std::string interval_str =
        command_line.GetSwitchValueASCII(switches::kTraceMemoryInterval);
    if (!interval_str.empty()) {
      uint32_t timer_period_secs;
      if (!base::StringToUint(interval_str, &timer_period_secs)) {
        LOG(ERROR) << "Failed to parse --trace-memory-interval="
                   << timer_period_secs;
      } else {
        timer_period_secs_ = timer_period_secs;
      }
    }
  }

  bool use_mega_bytes = false;

  if (command_line.HasSwitch(switches::kTraceMemoryByteUnit)) {
    std::string byteunit_str =
        command_line.GetSwitchValueASCII(switches::kTraceMemoryByteUnit);

    if (byteunit_str.empty() || byteunit_str == "kb" || byteunit_str == "KB")
      use_mega_bytes = false;

    if (byteunit_str == "mb" || byteunit_str == "MB")
      use_mega_bytes = true;
  }

  bool is_trace_log_csv = false;

  if (command_line.HasSwitch(switches::kTraceMemoryLogFormat)) {
    std::string log_format =
        command_line.GetSwitchValueASCII(switches::kTraceMemoryLogFormat);

    if (log_format == "csv")
      is_trace_log_csv = true;
  }

  // Default trace output file is stderr.
  FILE* trace_file = stderr;

  if (command_line.HasSwitch(switches::kTraceMemoryToFile)) {
    base::FilePath trace_file_path =
        command_line.GetSwitchValuePath(switches::kTraceMemoryToFile);

    if (trace_file_path.empty()) {
      // Default to saving the memory trace into the current dir.
      trace_file_path = base::FilePath().AppendASCII("trace-memory.log");
    }

    // Add its PID to the given file name as a postfix.
    trace_file_path = trace_file_path.AddExtension(base::IntToString(getpid()));

    if (is_trace_log_csv) {
      // Add trace memory file format as a postfix.
      trace_file_path = trace_file_path.AddExtension("csv");
    }

    trace_file = OpenFile(trace_file_path, "w");
  }

  trace_file_ = trace_file;
  is_trace_log_csv_ = is_trace_log_csv;
  use_mega_bytes_ = use_mega_bytes;

  return true;
}

MemoryTraceManager::MemoryTraceProviderInfo::MemoryTraceProviderInfo(
    MemoryTraceProvider* trace_provider,
    const char* name,
    scoped_refptr<SequencedTaskRunner> task_runner)
    : trace_provider(trace_provider),
      name(name),
      task_runner(std::move(task_runner)),
      disabled(false) {}

MemoryTraceManager::MemoryTraceProviderInfo::~MemoryTraceProviderInfo() {}

bool MemoryTraceManager::MemoryTraceProviderInfo::Comparator::operator()(
    const scoped_refptr<MemoryTraceManager::MemoryTraceProviderInfo>& a,
    const scoped_refptr<MemoryTraceManager::MemoryTraceProviderInfo>& b) const {
  if (!a || !b)
    return a.get() < b.get();
  // Ensure that unbound providers (task_runner == nullptr) always run last.
  // Rationale: some unbound trace providers are known to be slow, keep them
  // last to avoid skewing timings of the other trace providers.
  return std::tie(a->task_runner, a->trace_provider) >
         std::tie(b->task_runner, b->trace_provider);
}

MemoryTraceManager::PeriodicGlobalTraceTimer::PeriodicGlobalTraceTimer() {}

MemoryTraceManager::PeriodicGlobalTraceTimer::~PeriodicGlobalTraceTimer() {
  Stop();
}

void MemoryTraceManager::PeriodicGlobalTraceTimer::Start(
    uint32_t timer_period_secs) {
  timer_.Start(FROM_HERE, TimeDelta::FromSeconds(timer_period_secs),
               base::Bind(&PeriodicGlobalTraceTimer::RequestPeriodicGlobalTrace,
                          base::Unretained(this)));
}

void MemoryTraceManager::PeriodicGlobalTraceTimer::Stop() {
  if (IsRunning()) {
    timer_.Stop();
  }
}

bool MemoryTraceManager::PeriodicGlobalTraceTimer::IsRunning() {
  return timer_.IsRunning();
}

void MemoryTraceManager::PeriodicGlobalTraceTimer::RequestPeriodicGlobalTrace() {
  MemoryTraceManager::GetInstance()->RequestGlobalTrace();
}

bool MemoryTraceManagerDelegate::Initialize(bool is_browser_process) {
  return base::trace_event::neva::MemoryTraceManager::GetInstance()
      ->Initialize(this /* delegate */, is_browser_process);
}

void MemoryTraceManagerDelegate::RequestGlobalMemoryTrace(
    const MemoryTraceCallback& callback) {
  callback.Run(false /* success */);
  MemoryTraceManager::GetInstance()->InvokeOnMemoryTrace();
}

}  // namespace neva
}  // namespace trace_event
}  // namespace base
