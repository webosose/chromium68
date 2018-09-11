// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_extension_web_contents_observer.h"

#include "base/command_line.h"
#include "base/json/string_escape.h"
#include "base/strings/utf_string_conversions.h"
#include "content/common/frame_messages.h"
#include "content/public/browser/navigation_handle.h"
#include "content/public/browser/render_frame_host.h"

DEFINE_WEB_CONTENTS_USER_DATA_KEY(
    extensions::ShellExtensionWebContentsObserver);

namespace extensions {

ShellExtensionWebContentsObserver::ShellExtensionWebContentsObserver(
    content::WebContents* web_contents)
    : ExtensionWebContentsObserver(web_contents) {
}

ShellExtensionWebContentsObserver::~ShellExtensionWebContentsObserver() {
}

void ShellExtensionWebContentsObserver::DidFinishNavigation(
    content::NavigationHandle* navigation_handle) {
  ExtensionWebContentsObserver::DidFinishNavigation(navigation_handle);

  if (!navigation_handle->HasCommitted())
    return;

  content::RenderFrameHost* render_frame_host =
      navigation_handle->GetRenderFrameHost();
  bool is_extension = GetExtensionFromFrame(render_frame_host, true);
  const char* launchArgsSwitch = "launch-args";
  if (is_extension &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(launchArgsSwitch)) {
    std::string js_line =
        "chrome.app.launchArgs=" +
        base::GetQuotedJSONString(
            base::CommandLine::ForCurrentProcess()->GetSwitchValueNative(
                launchArgsSwitch)) +
        ";";

    render_frame_host->Send(new FrameMsg_JavaScriptExecuteRequest(
        render_frame_host->GetRoutingID(), base::UTF8ToUTF16(js_line), 0,
        false));
  }
}

void ShellExtensionWebContentsObserver::CreateForWebContents(
    content::WebContents* web_contents) {
  content::WebContentsUserData<
      ShellExtensionWebContentsObserver>::CreateForWebContents(web_contents);

  // Initialize this instance if necessary.
  FromWebContents(web_contents)->Initialize();
}

}  // namespace extensions
