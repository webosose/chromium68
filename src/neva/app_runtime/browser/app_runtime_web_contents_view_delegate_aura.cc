// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

namespace content {
class WebContents;
class WebContentsViewDelegate;
}

namespace app_runtime {

content::WebContentsViewDelegate* CreateAppRuntimeWebContentsViewDelegate(
    content::WebContents* web_contents) {
  return nullptr;
}

}  // namespace app_runtime
