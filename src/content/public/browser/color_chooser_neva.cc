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

#include "content/public/browser/color_chooser_neva.h"

#include "base/json/json_writer.h"
#include "base/json/string_escape.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"

namespace neva {
ColorChooser* ColorChooser::current_color_chooser_ = nullptr;

ColorChooser::ColorChooser(
    content::WebContents* web_contents, SkColor initial_color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions,
    pal::NativeControlsInterface* interface)
    : web_contents_(web_contents), weak_ptr_factory_(this) {
  using namespace base;
  std::unique_ptr<ListValue> suggestion_list(new ListValue);
  DictionaryValue params;
  params.SetInteger("color", initial_color);
  for (auto const& suggestion : suggestions) {
    std::unique_ptr<DictionaryValue> suggestion_dict(new DictionaryValue);
    suggestion_dict->SetString("label", suggestion->label);
    suggestion_dict->SetInteger("color", suggestion->color);
    suggestion_list->Append(std::move(suggestion_dict));
  }
  params.Set("suggestions", std::move(suggestion_list));
  std::string params_str;
  base::JSONWriter::Write(params, &params_str);
  std::string esc_params;
  base::EscapeJSONString(params_str, false, &esc_params);

  color_changed_subscription_ = interface->AddCallback(
      base::Bind(&ColorChooser::OnColorChosen,weak_ptr_factory_.GetWeakPtr()));

  interface->OpenColorChooser(
      esc_params, base::Bind(&ColorChooser::OnColorChooserDialogClosed,
                             weak_ptr_factory_.GetWeakPtr()));
}

ColorChooser* ColorChooser::Open(
    content::WebContents* web_contents, SkColor initial_color,
    const std::vector<blink::mojom::ColorSuggestionPtr>& suggestions) {
  if (current_color_chooser_) return nullptr;
  pal::NativeControlsInterface* interface =
      pal::Pal::GetPlatformInstance()->GetNativeControlsInterface();
  if (interface) {
    current_color_chooser_ =
        new ColorChooser(web_contents, initial_color, suggestions, interface);
    return current_color_chooser_;
  } else {
    LOG(ERROR) << "Interface not available";
    return nullptr;
  }
}

ColorChooser::~ColorChooser() {}

void ColorChooser::End() {
  pal::NativeControlsInterface* interface =
      pal::Pal::GetPlatformInstance()->GetNativeControlsInterface();
  if (interface) {
    interface->CloseColorChooser();
  } else {
    LOG(ERROR) << "Interface not available";
  }

  OnColorChooserDialogClosed();
}

void ColorChooser::OnColorChosen(int color) {
  if (web_contents_)
    web_contents_->DidChooseColorInColorChooser((SkColor)color);
}

void ColorChooser::OnColorChooserDialogClosed() {
  DCHECK(current_color_chooser_ == this);
  current_color_chooser_ = NULL;
  color_changed_subscription_.reset();
  if (web_contents_) web_contents_->DidEndColorChooser();
}

}  // namespace neva
