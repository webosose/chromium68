// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_
#define NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_

namespace content {
class WebContents;
class WebContentsViewDelegate;
}

namespace app_runtime {

content::WebContentsViewDelegate* CreateAppRuntimeWebContentsViewDelegate(
    content::WebContents* web_contents);

}  // namespace app_runtime

#endif  // NEVA_APP_RUNTIME_BROWSER_APP_RUNTIME_WEB_CONTENTS_VIEW_DELEGATE_CREATOR_H_
