// Copyright (c) 2018 LG Electronics, Inc.
//
// Confidential computer software. Valid license from LG required for
// possession, use or copying. Consistent with FAR 12.211 and 12.212,
// Commercial Computer Software, Computer Software Documentation, and
// Technical Data for Commercial Items are licensed to the U.S. Government
// under vendor's standard commercial license.

#ifndef INJECTION_MEMORYMANAGER_MEMORYMANAGER_INJECTION_H_
#define INJECTION_MEMORYMANAGER_MEMORYMANAGER_INJECTION_H_

#include <string>

#include "base/compiler_specific.h"

#if defined(COMPONENT_BUILD)
#define MEMORYMANAGER_INJECTION_EXPORT __attribute__((visibility("default")))
#else
#define MEMORYMANAGER_INJECTION_EXPORT
#endif  // defined(COMPONENT_BUILD)

namespace blink {
class WebLocalFrame;
}  // namespace blink

namespace extensions_v8 {

class InjectionWrapper;

class MEMORYMANAGER_INJECTION_EXPORT MemoryManagerInjectionExtension {
 public:
  static InjectionWrapper* Get();
  static std::string GetNavigatorExtensionScript();

  static void InstallExtension(blink::WebLocalFrame *web_frame);

  static const char kInjectionName[];
};

}  // namespace extensions_v8

#endif  // INJECTION_MEMORYMANAGER_MEMORYMANAGER_INJECTION_H_
