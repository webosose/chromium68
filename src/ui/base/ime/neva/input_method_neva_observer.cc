// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#include "ui/base/ime/neva/input_method_neva_observer.h"
#include "base/strings/utf_string_conversions.h"
#include "ui/base/ime/text_input_client.h"

namespace ui {

InputMethodNevaObserver::InputMethodNevaObserver() {
}

InputMethodNevaObserver::~InputMethodNevaObserver() {
}

void InputMethodNevaObserver::OnFocus() {
}

void InputMethodNevaObserver::OnBlur() {
}

void InputMethodNevaObserver::OnCaretBoundsChanged(const TextInputClient* client) {
  gfx::Range text_range;
  gfx::Range selection_range;
  base::string16 surrounding_text;
  if (!client->GetTextRange(&text_range) ||
      !client->GetTextFromRange(text_range, &surrounding_text) ||
      !client->GetSelectionRange(&selection_range)) {
    return;
  }

  if (!selection_range.IsValid())
    return;

  // Here SetSurroundingText accepts relative position of |surrounding_text|, so
  // we have to convert |selection_range| from node coordinates to
  // |surrounding_text| coordinates.

  std::string text = base::UTF16ToUTF8(surrounding_text);
  size_t anchor_position = selection_range.start() - text_range.start();
  size_t cursor_position = selection_range.end() - text_range.start();

  SetSurroundingText(text, cursor_position, anchor_position);
}

void InputMethodNevaObserver::SetImeEnabled(bool enable) {
  is_enabled_ = enable;
}

void InputMethodNevaObserver::OnTextInputStateChanged(const TextInputClient* client) {
  if (!is_enabled_)
    return;

  if (client) {
    if (client->GetTextInputType() != TEXT_INPUT_TYPE_NONE) {
      OnTextInputTypeChanged(client->GetTextInputType(), client->GetTextInputFlags());
      if (!client->SystemKeyboardDisabled())
        OnShowIme();
    } else {
      OnHideIme();
    }
  }
}

void InputMethodNevaObserver::OnInputMethodDestroyed(const InputMethod* input_method) {
  if (!is_enabled_)
    return;

  OnHideIme();
}

void InputMethodNevaObserver::OnShowImeIfNeeded() {
}

}  // namespace ui
