// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/shell/browser/shell_app_delegate.h"

#include "content/public/browser/web_contents.h"
#include "extensions/browser/media_capture_util.h"
#include "extensions/common/constants.h"
#include "extensions/shell/browser/shell_extension_web_contents_observer.h"

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
#include "content/public/browser/render_view_host.h"
#include "neva/neva_chromium/content/common/injection_messages.h"
#include "pal/public/pal.h"
#endif

namespace extensions {

ShellAppDelegate::ShellAppDelegate() {
}

ShellAppDelegate::~ShellAppDelegate() {
}

void ShellAppDelegate::InitWebContents(content::WebContents* web_contents) {
#if defined(USE_NEVA_APPRUNTIME)
  web_contents->EnableInspectable();
#endif
  ShellExtensionWebContentsObserver::CreateForWebContents(web_contents);
}

void ShellAppDelegate::RenderViewCreated(
    content::RenderViewHost* render_view_host) {
  // The views implementation of AppWindow takes focus via SetInitialFocus()
  // and views::WebView but app_shell is aura-only and must do it manually.
  content::WebContents::FromRenderViewHost(render_view_host)->Focus();

#if defined(ENABLE_MEMORYMANAGER_WEBAPI)
  if (pal::Pal::GetPlatformInstance()->GetMemoryManagerInterface()) {
    render_view_host->Send(
        new InjectionMsg_LoadExtension(
            render_view_host->GetRoutingID(), std::string("v8/memorymanager")));
  }
#endif
}

void ShellAppDelegate::ResizeWebContents(content::WebContents* web_contents,
                                         const gfx::Size& size) {
  NOTIMPLEMENTED();
}

content::WebContents* ShellAppDelegate::OpenURLFromTab(
    content::BrowserContext* context,
    content::WebContents* source,
    const content::OpenURLParams& params) {
  NOTIMPLEMENTED();
  return NULL;
}

void ShellAppDelegate::AddNewContents(
    content::BrowserContext* context,
    std::unique_ptr<content::WebContents> new_contents,
    WindowOpenDisposition disposition,
    const gfx::Rect& initial_rect,
    bool user_gesture) {
  NOTIMPLEMENTED();
}

content::ColorChooser* ShellAppDelegate::ShowColorChooser(
    content::WebContents* web_contents,
    SkColor initial_color) {
  NOTIMPLEMENTED();
  return NULL;
}

void ShellAppDelegate::RunFileChooser(
    content::RenderFrameHost* render_frame_host,
    const content::FileChooserParams& params) {
  NOTIMPLEMENTED();
}

void ShellAppDelegate::RequestMediaAccessPermission(
    content::WebContents* web_contents,
    const content::MediaStreamRequest& request,
    const content::MediaResponseCallback& callback,
    const extensions::Extension* extension) {
  media_capture_util::GrantMediaStreamRequest(
      web_contents, request, callback, extension);
}

bool ShellAppDelegate::CheckMediaAccessPermission(
    content::RenderFrameHost* render_frame_host,
    const GURL& security_origin,
    content::MediaStreamType type,
    const Extension* extension) {
  if (type == content::MEDIA_DEVICE_AUDIO_CAPTURE ||
      type == content::MEDIA_DEVICE_VIDEO_CAPTURE) {
    // VerifyMediaAccessPermission() will crash if there is
    // no permission for audio capture / video capture.
    // Let's make an error log and return false instead.
    // TODO(alexander.trofimov@lge.com): Remove this patch
    // right after corresponding features are supported
    // and crash removed from VerifyMediaAccessPermission().
    LOG(ERROR) << "Audio capture/video capture request but "
               << "this feature is not supported yet.";
    return false;
  }
  media_capture_util::VerifyMediaAccessPermission(type, extension);
  return true;
}

int ShellAppDelegate::PreferredIconSize() const {
  return extension_misc::EXTENSION_ICON_SMALL;
}

void ShellAppDelegate::SetWebContentsBlocked(
    content::WebContents* web_contents,
    bool blocked) {
  NOTIMPLEMENTED();
}

bool ShellAppDelegate::IsWebContentsVisible(
    content::WebContents* web_contents) {
  return true;
}

void ShellAppDelegate::SetTerminatingCallback(const base::Closure& callback) {
  // TODO(jamescook): Should app_shell continue to close the app window
  // manually or should it use a browser termination callback like Chrome?
}

bool ShellAppDelegate::TakeFocus(content::WebContents* web_contents,
                                 bool reverse) {
  return false;
}

}  // namespace extensions
