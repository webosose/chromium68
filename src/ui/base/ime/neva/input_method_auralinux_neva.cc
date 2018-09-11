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

#include "ui/base/ime/neva/input_method_auralinux_neva.h"
#include "ui/base/ime/text_input_client.h"

namespace ui {

InputMethodAuraLinuxNeva::InputMethodAuraLinuxNeva(
    internal::InputMethodDelegate* delegate,
    unsigned handle)
    : InputMethodAuraLinux(delegate, handle) {
}

InputMethodAuraLinuxNeva::~InputMethodAuraLinuxNeva() {
}

void InputMethodAuraLinuxNeva::OnDeleteRange(int32_t index, uint32_t length) {
  if (IsTextInputTypeNone())
    return;

  TextInputClient* client = GetTextInputClient();
  if (client) {
    gfx::Range range;
    bool res = false;
    if (length != std::numeric_limits<uint32_t>::max()) {
      res = client->GetSelectionRange(&range);
      if (res) {
        range.set_start(range.GetMax() + index);
        range.set_end(range.start() + length);
      }
    } else {
      res = client->GetTextRange(&range);
    }

    if (res)
      client->DeleteRange(range);
  }
}

void InputMethodAuraLinuxNeva::OnCommit(const base::string16& text) {
  // In case WebOS we should reset suppress_non_key_input_until_ for OnCommit handling.
  // WebOS VKB calls OnCommit without key event processing in InputMethod and therefore
  // the first call of OnCommit is ignored.
#if defined(OS_WEBOS)
  suppress_non_key_input_until_ = base::TimeTicks::UnixEpoch();
#endif
  InputMethodAuraLinux::OnCommit(text);
}

}  // namespace ui
