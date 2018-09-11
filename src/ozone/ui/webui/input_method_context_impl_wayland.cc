// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/input_method_context_impl_wayland.h"

#include "base/logging.h"
#include "base/strings/utf_string_conversions.h"
#include "ozone/ui/webui/input_method_context_manager.h"
#include "ui/base/ime/composition_text.h"

namespace ui {

InputMethodContextImplWayland::InputMethodContextImplWayland(
    LinuxInputMethodContextDelegate* delegate,
    unsigned handle,
    ui::InputMethodContextManager* input_context_manager)
    : delegate_(delegate),
      input_method_context_manager_(input_context_manager),
      handle_(handle) {
  CHECK(delegate_);
  input_method_context_manager_->AddContext(this);
}

InputMethodContextImplWayland::~InputMethodContextImplWayland() {
  input_method_context_manager_->RemoveContext(this);
}

////////////////////////////////////////////////////////////////////////////////
// InputMethodContextImplWayland, ui::LinuxInputMethodContext implementation:
bool InputMethodContextImplWayland::DispatchKeyEvent(
    const KeyEvent& key_event) {
  return false;
}

void InputMethodContextImplWayland::Reset() {
  input_method_context_manager_->ImeReset();
}

void InputMethodContextImplWayland::Focus() {
}

void InputMethodContextImplWayland::Blur() {
}

void InputMethodContextImplWayland::SetCursorLocation(const gfx::Rect&) {
}

void InputMethodContextImplWayland::Commit(const std::string& text) {
  delegate_->OnCommit(base::string16(base::ASCIIToUTF16(text.c_str())));
}

void InputMethodContextImplWayland::PreeditChanged(const std::string& text,
                                                   const std::string& commit) {
  ui::CompositionText composition_text;
  composition_text.text = base::string16(base::ASCIIToUTF16(text.c_str()));
  delegate_->OnPreeditChanged(composition_text);
}

void InputMethodContextImplWayland::DeleteRange(int32_t index,
                                                uint32_t length) {
  delegate_->OnDeleteRange(index, length);
}

}  // namespace ui
