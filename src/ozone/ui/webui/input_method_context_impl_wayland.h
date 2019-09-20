// Copyright 2014 Intel Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_
#define OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_

#include <string>

#include "ozone/platform/ozone_export_wayland.h"
#include "ui/base/ime/linux/linux_input_method_context.h"
#include "ui/gfx/geometry/rect.h"

namespace ui {

class InputMethodContextManager;

// An implementation of LinuxInputMethodContext for IME support on Ozone
// platform using Wayland.
class OZONE_WAYLAND_EXPORT InputMethodContextImplWayland
  : public LinuxInputMethodContext {
 public:
  InputMethodContextImplWayland(ui::LinuxInputMethodContextDelegate* delegate,
                                unsigned handle,
                                ui::InputMethodContextManager* input_context_manager);
  ~InputMethodContextImplWayland() override;

  // overriden from ui::LinuxInputMethodContext
  bool DispatchKeyEvent(const ui::KeyEvent& key_event) override;
  void Reset() override;
  void Focus() override;
  void Blur() override;
  void SetCursorLocation(const gfx::Rect&) override;
  void SetSurroundingText(const base::string16& text,
                          const gfx::Range& selection_range) override;

  unsigned GetHandle() const { return handle_; }

  void Commit(const std::string& text);
  void PreeditChanged(const std::string& text,
                      const std::string& commit);
  void DeleteRange(int32_t index, uint32_t length);

 private:
  // Must not be NULL.
  LinuxInputMethodContextDelegate* delegate_;
  InputMethodContextManager* input_method_context_manager_;
  unsigned handle_;
  DISALLOW_COPY_AND_ASSIGN(InputMethodContextImplWayland);
};

}  // namespace ui

#endif  //  OZONE_UI_WEBUI_INPUT_METHOD_CONTEXT_IMPL_WAYLAND_H_
