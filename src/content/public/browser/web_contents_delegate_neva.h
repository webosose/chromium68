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

#ifndef CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_NEVA_H_
#define CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_NEVA_H_

#include "content/public/browser/web_contents_delegate.h"

namespace content {
namespace neva {

class CONTENT_EXPORT WebContentsDelegate : public content::WebContentsDelegate {
 public:
  WebContentsDelegate();

  // Called when color chooser should open. Returns the opened color chooser.
  // Returns nullptr if we failed to open the color chooser (e.g. when there is
  // a ColorChooserDialog already open on Windows). Ownership of the returned
  // pointer is transferred to the caller.
  content::ColorChooser* OpenColorChooser(
      WebContents* web_contents,
      SkColor color,
      const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions)
      override;

  // Called when a file selection is to be done.
  void RunFileChooser(RenderFrameHost* render_frame_host,
                      const FileChooserParams& params) override;

 protected:
  ~WebContentsDelegate() override;

  void OnRunFileChooserRespondCallback(RenderFrameHost* render_frame_host,
                                       std::string selected_files);

 private:
  base::WeakPtrFactory<WebContentsDelegate> weak_ptr_factory_;
};

}  // namespace neva
}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_WEB_CONTENTS_DELEGATE_NEVA_H_
