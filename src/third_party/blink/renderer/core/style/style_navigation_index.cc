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

#include "third_party/blink/renderer/core/style/style_navigation_index.h"

namespace blink {

StyleNavigationIndex::StyleNavigationIndex() : is_auto(true) {}

StyleNavigationIndex::StyleNavigationIndex(const StyleNavigationIndex& o)
    : RefCounted<StyleNavigationIndex>(), index(o.index), is_auto(o.is_auto) {}

bool StyleNavigationIndex::operator==(const StyleNavigationIndex& o) const {
  return ((is_auto || o.is_auto) ? is_auto == o.is_auto : index == o.index);
}

// static
scoped_refptr<StyleNavigationIndex> StyleNavigationIndex::Create() {
  return base::AdoptRef(new StyleNavigationIndex);
}

scoped_refptr<StyleNavigationIndex> StyleNavigationIndex::Copy() const {
  return base::AdoptRef(new StyleNavigationIndex(*this));
}

}  // namespace blink
