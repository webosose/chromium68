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

#ifndef CONTENT_PUBLIC_BROWSER_COLOR_CHOOSER_NEVA_H_
#define CONTENT_PUBLIC_BROWSER_COLOR_CHOOSER_NEVA_H_

#include "content/public/browser/color_chooser.h"
#include "content/public/browser/web_contents.h"
#include "pal/ipc/pal_macros.h"
#include "pal/public/pal.h"
#include "third_party/blink/public/mojom/color_chooser/color_chooser.mojom.h"

namespace neva {

class ColorChooser : public content::ColorChooser {
 public:
  static ColorChooser* Open(
      content::WebContents* web_contents, SkColor initial_color,
      const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions);

  ~ColorChooser() override;

  void End() override;
  void SetSelectedColor(SkColor color) override {}

 private:
  ColorChooser(content::WebContents* web_contents, SkColor initial_color,
               const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions,
               pal::NativeControlsInterface* interface);

  void OnColorChosen(int color);
  void OnColorChooserDialogClosed();

  static ColorChooser* current_color_chooser_;
  content::WebContents* web_contents_;
  std::unique_ptr<pal::NativeControlsInterface::ColorChosenSubscription>
      color_changed_subscription_;

  base::WeakPtrFactory<ColorChooser> weak_ptr_factory_;
};

}  // namespace neva

#endif  // CONTENT_PUBLIC_BROWSER_COLOR_CHOOSER_NEVA_H_
