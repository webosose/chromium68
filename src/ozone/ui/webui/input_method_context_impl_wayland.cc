// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ozone/ui/webui/input_method_context_impl_wayland.h"

#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "ozone/ui/webui/input_method_context_manager.h"
#include "ui/base/ime/composition_text.h"

constexpr SkColor kPreeditHighlightColor =
#if defined(OS_WEBOS)
    // specified by UX team
    SkColorSetARGB(0xFF, 198, 176, 186);
#else
    SK_ColorTRANSPARENT;
#endif

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

void InputMethodContextImplWayland::SetSurroundingText(
    const base::string16& text,
    const gfx::Range& selection_range) {}

void InputMethodContextImplWayland::Commit(const std::string& text) {
  base::string16 string_commited;
  if (base::IsStringUTF8(text))
    base::UTF8ToUTF16(text.c_str(), text.length(), &string_commited);
  else
    string_commited = base::ASCIIToUTF16(text);

  delegate_->OnCommit(string_commited);
}

void InputMethodContextImplWayland::PreeditChanged(const std::string& text,
                                                   const std::string& commit) {
  ui::CompositionText composition_text;
  if (base::IsStringUTF8(text)) {
    base::UTF8ToUTF16(text.c_str(), text.length(), &composition_text.text);
    composition_text.selection = gfx::Range(0, composition_text.text.length());
    composition_text.ime_text_spans.push_back(ui::ImeTextSpan(
        ui::ImeTextSpan::Type::kComposition, 0, composition_text.text.length(),
        ui::ImeTextSpan::Thickness::kNone, kPreeditHighlightColor));
  } else
    composition_text.text = base::ASCIIToUTF16(text);

  delegate_->OnPreeditChanged(composition_text);
}

void InputMethodContextImplWayland::DeleteRange(int32_t index,
                                                uint32_t length) {
  delegate_->OnDeleteRange(index, length);
}

}  // namespace ui
