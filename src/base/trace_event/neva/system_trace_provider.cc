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

#include "base/trace_event/neva/memory_trace_manager.h"
#include "base/trace_event/neva/system_trace_provider.h"

#include <stddef.h>
#include <unistd.h>

#include "base/strings/safe_sprintf.h"
#include "build/build_config.h"

namespace base {
namespace trace_event {
namespace neva {

// static
SystemTraceProvider* SystemTraceProvider::GetInstance() {
  return Singleton<SystemTraceProvider,
                   LeakySingletonTraits<SystemTraceProvider>>::get();
}

SystemTraceProvider::SystemTraceProvider() {}

SystemTraceProvider::~SystemTraceProvider() {}

// Simple wrapper for strncmp without length.
static inline bool StrCmp(const char* s1, const char* s2) {
  return strncmp(s1, s2, strlen(s2));
}

// Extract a value in a field seperated line as follows:
//   MemFree:           28720 kB
//   MemAvailable:     223296 kB
//   Pss:                  12 kB
static int GetValueFromLine(const char* line, const char* field) {
  const char* p = line + strlen(field);
  char buf[32];

  while (*(++p) == ' ');        // skip white spaces
  strncpy(buf, p, 32);
  buf[strlen(buf) - 4] = '\0';  // remove a unit from "16 kB\n"

  return atoi(buf);
}

class ProcStatMReader {
 private:
  struct StatMInfo {
    int size;
    int resident;
    int shared;
  };

  bool ReadAndSetStatM() {
    const char* statm_file = "/proc/self/statm";
    int pagesize_kb = sysconf(_SC_PAGESIZE) / KB;
    int size, resident, shared;

    FILE* fp = fopen(statm_file, "r");
    if (fscanf(fp, "%d %d %d", &size, &resident, &shared) == EOF) {
      fclose(fp);
      return false;
    }
    fclose(fp);

    statm_.size     = size     * pagesize_kb;
    statm_.resident = resident * pagesize_kb;
    statm_.shared   = shared   * pagesize_kb;

    has_read_ = true;
    return true;
  }

 public:
  ProcStatMReader() : has_read_(false) {}

  inline void ReadAndSetStatMIfNeeded() {
    if (has_read_ == false)
      ReadAndSetStatM();
  }

  inline int GetVSS() {
    ReadAndSetStatMIfNeeded();
    return statm_.size;
  }

  inline int GetRSS() {
    ReadAndSetStatMIfNeeded();
    return statm_.resident;
  }

  inline int GetShared() {
    ReadAndSetStatMIfNeeded();
    return statm_.shared;
  }

 private:
  bool has_read_;
  struct StatMInfo statm_;
};

class ProcSmapsReader {
 private:
  enum SmapsCategory {
    SMAPS_CAT_FILE,
    SMAPS_CAT_SHARED,
    SMAPS_CAT_ANON,
    SMAPS_CAT_STACK,
    SMAPS_CAT_ETC,

    SMAPS_CAT_TOTAL,
  };

  struct SmapsInfo {
    int file;
    int shared;
    int anon;
    int stack;
    int etc;
  };

  inline char* MoveToNextField(char* p) {
    while (*p != 0 && *p != ' ')
      ++p;

    // skip whitespaces
    for ( ; *p == ' '; p++);
    return p;
  }

  inline bool IsSharedMap(char* p) { return p[3] == 's'; }

  void ReadAndSetPSSForEachCategory() {
    const char* smaps_file = "/proc/self/smaps";
    int total_pss[SMAPS_CAT_TOTAL] = { 0, };
    int total_swap = 0;
    enum SmapsCategory cat;
    char line[1024];
    FILE* fsmaps;

    fsmaps = fopen(smaps_file, "r");

    while (fgets(line, 1024, fsmaps)) {
      if (IsAddressHeadLine(line)) {
        // Find the category of page.
        char* p = line;
        bool is_shared = false;

        // 00400000-0040c000 r-xp 00000000 08:32 919321         /bin/cat
        p = MoveToNextField(p);  // Move to permission field
        if (IsSharedMap(p))
          is_shared = true;
        p = MoveToNextField(p);
        p = MoveToNextField(p);
        p = MoveToNextField(p);
        p = MoveToNextField(p);

        p[strlen(p) - 1] = '\0';
        cat = GetCategory(p);
        if (is_shared)
          cat = SMAPS_CAT_SHARED;
      } else if (!StrCmp(line, "Pss")) {
        total_pss[cat] += GetValueFromLine(line, "Pss");
      } else if (!StrCmp(line, "Swap")) {
        total_swap += GetValueFromLine(line, "Swap");
      }
    }

    fclose(fsmaps);

    smaps_pss_.file = total_pss[SMAPS_CAT_FILE];
    smaps_pss_.shared = total_pss[SMAPS_CAT_SHARED];
    smaps_pss_.anon = total_pss[SMAPS_CAT_ANON];
    smaps_pss_.stack = total_pss[SMAPS_CAT_STACK];
    smaps_pss_.etc = total_pss[SMAPS_CAT_ETC];

    smaps_swap_ = total_swap;

    has_read_ = true;
  }

  inline bool IsAddressHeadLine(const char* line) {
    if ((line[0] >= '0' && line[0] <= '9') ||
        (line[0] >= 'a' && line[0] <= 'f')) {
      return true;
    }
    return false;
  }

  enum SmapsCategory GetCategory(const char* p) {
    enum SmapsCategory cat;
    switch (*p) {
      case 0:
        cat = SMAPS_CAT_ANON;
        break;
      case '/':
        cat = SMAPS_CAT_FILE;
        break;
      case '[':
        if (!strcmp(p, "[heap]"))
          cat = SMAPS_CAT_ANON;
        else if (!strcmp(p, "[stack]"))
          cat = SMAPS_CAT_STACK;
        else
          cat = SMAPS_CAT_ETC;
        break;
      default:
        abort();
    }
    return cat;
  }

 public:
  ProcSmapsReader() : has_read_(false) {}

  inline void ReadAndSetPSSForEachCategoryIfNeeded() {
    if (has_read_ == false)
      ReadAndSetPSSForEachCategory();
  }

  inline int GetPSSTotal() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.file + smaps_pss_.shared + smaps_pss_.anon +
           smaps_pss_.stack + smaps_pss_.etc;
  }

  inline int GetPSSOfFile() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.file;
  }

  inline int GetPSSOfShared() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.shared;
  }

  inline int GetPSSOfAnon() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.anon;
  }

  inline int GetPSSOfStack() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.stack;
  }

  inline int GetPSSOfEtc() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_pss_.etc;
  }

  inline int GetSwap() {
    ReadAndSetPSSForEachCategoryIfNeeded();
    return smaps_swap_;
  }

 private:
  bool has_read_;
  struct SmapsInfo smaps_pss_;
  int smaps_swap_;
};

class ProcMemInfoReader {
 private:
  void ReadAndSetMemInfo() {
    const char* meminfo_file = "/proc/meminfo";
    char line[1024];
    FILE* fmeminfo;
    int mem_free;
    int mem_available;
    int swap_free;

    fmeminfo= fopen(meminfo_file, "r");

    while (fgets(line, 1024, fmeminfo)) {
      if (!StrCmp(line, "MemFree"))
        mem_free = GetValueFromLine(line, "MemFree");
      else if (!StrCmp(line, "MemAvailable"))
        mem_available = GetValueFromLine(line, "MemAvailable");
      else if (!StrCmp(line, "SwapFree"))
        swap_free = GetValueFromLine(line, "SwapFree");
    }

    fclose(fmeminfo);

    mem_free_ = mem_free;
    mem_available_ = mem_available;
    swap_free_ = swap_free;

    has_read_ = true;
  }

 public:
  ProcMemInfoReader() : has_read_(false) {}

  inline void ReadAndSetMemInfoIfNeeded() {
    if (has_read_ == false)
      ReadAndSetMemInfo();
  }

  inline int GetMemFree() {
    ReadAndSetMemInfoIfNeeded();
    return mem_free_;
  }

  inline int GetMemAvailable() {
    ReadAndSetMemInfoIfNeeded();
    return mem_available_;
  }

  inline int GetSwapFree() {
    ReadAndSetMemInfoIfNeeded();
    return swap_free_;
  }

 private:
  bool has_read_;
  int mem_free_;
  int mem_available_;
  int swap_free_;
};

// Called at trace dump point time. Creates a snapshot the memory counters for
// the current process.
bool SystemTraceProvider::OnMemoryTrace() {
  const char* print_fmt;
  MemoryTraceManager* mtm = MemoryTraceManager::GetInstance();
  FILE* trace_fp = mtm->GetTraceFile();
  bool is_trace_log_csv = mtm->IsTraceLogCSV();
  bool use_mega_bytes = mtm->GetUseMegaBytes();
  int mb = use_mega_bytes ? 1024 : 1;

  {
    // Read /proc/self/statm and print its info.
    ProcStatMReader proc_statm_reader;
    int vss    = proc_statm_reader.GetVSS() / mb;
    int rss    = proc_statm_reader.GetRSS() / mb;
    int shared = proc_statm_reader.GetShared() / mb;

    if (is_trace_log_csv) {
      print_fmt = "%d, %d, %d, ";
    } else {
      if (use_mega_bytes) {
        print_fmt = "[system] VSS = %4d MB, RSS = %4d MB, shared = %4d MB\n";
      } else {
        print_fmt = "[system] VSS = %8d KB, RSS = %8d KB, shared = %8d KB\n";
      }
    }
    fprintf(trace_fp, print_fmt, vss, rss, shared);
  }

  {
    // Read /proc/self/smaps and print its info.
    ProcSmapsReader proc_smaps_reader;
    int pss_total  = proc_smaps_reader.GetPSSTotal() / mb;
    int pss_file   = proc_smaps_reader.GetPSSOfFile() / mb;
    int pss_stack  = proc_smaps_reader.GetPSSOfStack() / mb;
    int pss_anon   = proc_smaps_reader.GetPSSOfAnon() / mb;
    int pss_shared = proc_smaps_reader.GetPSSOfShared() / mb;
    int pss_etc    = proc_smaps_reader.GetPSSOfEtc() / mb;
    int total_swap = proc_smaps_reader.GetSwap() / mb;

    if (is_trace_log_csv) {
      fprintf(trace_fp, "%d, %d, %d, %d, ",
              pss_total, pss_file, pss_anon, total_swap);
    } else {
      if (use_mega_bytes) {
        print_fmt = "[system] PSS = %4d MB (file = %4d, shared = %4d, "
                             "anon = %4d, stack = %4d, etc = %4d), "
                             "Swap = %4d MB\n";
      } else {
        print_fmt = "[system] PSS = %8d KB (file = %d, shared = %d, "
                             "anon = %d, stack = %d, etc = %d), "
                             "Swap = %d KB\n";
      }
      fprintf(trace_fp, print_fmt,
              pss_total, pss_file, pss_shared, pss_anon, pss_stack, pss_etc,
              total_swap);
    }
  }

  {
    // Read /proc/meminfo and print its info.
    ProcMemInfoReader proc_mem_info_reader;
    int mem_free = proc_mem_info_reader.GetMemFree() / mb;
    int mem_available = proc_mem_info_reader.GetMemAvailable() / mb;
    int swap_free = proc_mem_info_reader.GetSwapFree() / mb;

    if (is_trace_log_csv) {
      print_fmt = "%d, %d, %d";
    } else {
      if (use_mega_bytes) {
        print_fmt = "[system] MemFree = %4d MB, MemAvailable = %4d MB, SwapFree = %4d MB\n";
      } else {
        print_fmt = "[system] MemFree = %8d KB, MemAvailable = %8d KB, SwapFree = %8d KB\n";
      }
    }
    fprintf(trace_fp, print_fmt, mem_free, mem_available, swap_free);
  }

  return true;
}

std::string SystemTraceProvider::GetCSVHeader() {
  return std::string("VSS, RSS, shared, "
                     "PSS, PSS:file, PSS:anon, "
                     "Swap, MemFree, MemAvailable, SwapFree");
}

}  // namespace neva
}  // namespace trace_event
}  // namespace base
