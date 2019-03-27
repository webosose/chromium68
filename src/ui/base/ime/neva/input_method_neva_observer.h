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

#ifndef UI_BASE_IME_NEVA_INPUT_METHOD_MANAGER_H_
#define UI_BASE_IME_NEVA_INPUT_METHOD_MANAGER_H_

#include <string>

#include "ui/base/ime/input_method_observer.h"
#include "ui/base/ime/neva/input_method_common.h"
#include "ui/base/ime/text_input_type.h"
#include "ui/base/ime/ui_base_ime_export.h"

namespace ui {

class UI_BASE_IME_EXPORT InputMethodNevaObserver
    : public InputMethodObserver {
 public:
  InputMethodNevaObserver();
  ~InputMethodNevaObserver() override;

  // InputMethodObserver overrides:
  void OnFocus() override;
  void OnBlur() override;
  void OnCaretBoundsChanged(const TextInputClient* client) override;
  void OnTextInputStateChanged(const TextInputClient* client) override;
  void OnInputMethodDestroyed(const InputMethod* input_method) override;
  void OnShowImeIfNeeded() override;

  virtual void OnShowIme() = 0;
  virtual void OnHideIme(ImeHiddenType) = 0;
  virtual void OnTextInputTypeChanged(ui::TextInputType text_input_type,
                                      int text_input_flags) = 0;
  virtual void SetSurroundingText(const std::string& text,
                                  size_t cursor_position,
                                  size_t anchor_position) = 0;
  void SetImeEnabled(bool enable);
private:
  bool is_enabled_ = false;
};

}  // namespace ui

#endif  // UI_BASE_IME_NEVA_INPUT_METHOD_MANAGER_H_
