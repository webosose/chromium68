// Copyright (c) 2016-2018 LG Electronics, Inc.
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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_INDEX_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_INDEX_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class StyleNavigationIndex : public RefCounted<StyleNavigationIndex> {
 public:
  static scoped_refptr<StyleNavigationIndex> Create();
  scoped_refptr<StyleNavigationIndex> Copy() const;

  bool operator==(const StyleNavigationIndex& o) const;
  bool operator!=(const StyleNavigationIndex& o) const { return !(*this == o); }

  int index;
  bool is_auto;

 private:
  StyleNavigationIndex();
  explicit StyleNavigationIndex(const StyleNavigationIndex&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_INDEX_H_
