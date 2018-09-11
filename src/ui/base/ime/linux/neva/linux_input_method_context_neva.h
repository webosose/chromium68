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

#ifndef UI_BASE_IME_LINUX_NEVA_LINUX_INPUT_METHOD_CONTEXT_NEVA_H_
#define UI_BASE_IME_LINUX_NEVA_LINUX_INPUT_METHOD_CONTEXT_NEVA_H_

namespace ui {

class NevaLinuxInputMethodContextDelegate {
 public:
  ~NevaLinuxInputMethodContextDelegate() {}

  // Delete the text range in the text input client.
  virtual void OnDeleteRange(int32_t index, uint32_t length) {}
};

}  // namespace ui

#endif  // UI_BASE_IME_LINUX_NEVA_LINUX_INPUT_METHOD_CONTEXT_NEVA_H_
