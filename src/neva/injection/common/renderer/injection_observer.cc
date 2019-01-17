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

#include "base/command_line.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/content_neva_switches.h"
#include "content/public/renderer/render_frame.h"
#include "content/public/renderer/render_thread.h"
#include "content/renderer/render_view_impl.h"
#include "injection/common/renderer/injection_observer.h"
#include "injection/common/wrapper/injection_wrapper.h"
#include "neva_chromium/content/common/injection_messages.h"

//  If some WEBAPI is enabled include it's headers
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
#include "injection/browser_control/browser_control_injection.h"
#endif

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
#include "injection/memorymanager/memorymanager_injection.h"
#endif

#if defined(ENABLE_SAMPLE_WEBAPI)
#include "injection/sample/sample_injection.h"
#endif

namespace content {

InjectionObserver::InjectionObserver(RenderViewImpl* render_view)
    : RenderViewObserver(render_view) {
// Loads extension for injection
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableBrowserControlInjection)) {
    OnLoadExtension(
        extensions_v8::BrowserControlInjectionExtension::kInjectionName);
  }
#endif  // ENABLE_BROWSER_CONTROL_WEBAPI
#if defined(ENABLE_SAMPLE_WEBAPI)
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kEnableSampleInjection)) {
    OnLoadExtension(extensions_v8::SampleInjectionExtension::kInjectionName);
  }
#endif  // ENABLE_SAMPLE_WEBAPI
}

InjectionObserver::~InjectionObserver() {}

bool InjectionObserver::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(InjectionObserver, message)
    IPC_MESSAGE_HANDLER(InjectionMsg_LoadExtension, OnLoadExtension)
    IPC_MESSAGE_HANDLER(InjectionMsg_ClearExtensions, OnClearExtensions)
    IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  return handled;
}

void InjectionObserver::OnDestruct() {
  delete this;
}

void InjectionObserver::OnLoadExtension(const std::string& extension) {
  RenderThread* thread = RenderThread::Get();
  if (thread) {
    extensions_v8::InjectionWrapper* const ext =
        extensions_v8::InjectionLoaderExtension::Get(extension);

    if (ext) {
      InitializeDispatcher(extension);
      thread->RegisterExtension(ext);
    }

    if (install_functions_.find(extension) == install_functions_.end()) {
      extensions_v8::InjectionInstallFunction install_function =
          extensions_v8::InjectionLoaderExtension::GetInjectionInstallFunction(
              extension);
      if (install_function) {
        install_functions_[extension] = install_function;
      } else {
        LOG(ERROR) << "The extension for injection is not created";
      }
    }
  } else {
    LOG(ERROR) << "RenderThread is NULL";
  }
}

void InjectionObserver::OnClearExtensions() {
  RenderThread* thread = RenderThread::Get();
  if (thread) {
    thread->ClearExtensions();
  }
  install_functions_.clear();
}

void InjectionObserver::InitializeDispatcher(const std::string& extension) {
  RenderFrame* mainRenderFrame = render_view()->GetMainRenderFrame();
  if (!mainRenderFrame)
    LOG(ERROR) << "Failed to create dispatcher because MainRenderFrame is NULL";
  //  Creates dispatcher for all enabled WEBAPI
  //  #if defined(ENABLE_NETWORK_INFO_WEBAPI)
  //  else if (extension == "network_info" && !network_info_observer_)
  //    network_info_observer_ = new NetworkInfoDispatcher(mainRenderFrame);
  //  #endif
}

void InjectionObserver::DidClearWindowObject(blink::WebLocalFrame* frame) {
  if (render_view()->GetMainRenderFrame()) {
    // if some WEBAPI is enable, get its main render frame
#if defined(ENABLE_BROWSER_CONTROL_WEBAPI)
    render_view()->GetMainRenderFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(extensions_v8::BrowserControlInjectionExtension::
                              GetNavigatorExtensionScript()));
#endif
#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
    render_view()->GetMainRenderFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(extensions_v8::MemoryManagerInjectionExtension::
                              GetNavigatorExtensionScript()));
#endif
#if defined(ENABLE_SAMPLE_WEBAPI)
    render_view()->GetMainRenderFrame()->ExecuteJavaScript(
        base::UTF8ToUTF16(extensions_v8::SampleInjectionExtension::
                              GetNavigatorExtensionScript()));
#endif
    for (auto iter : install_functions_) {
      (*iter.second)(frame);
    }
  }
}

}  // namespace content
