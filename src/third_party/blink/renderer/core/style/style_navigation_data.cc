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

#include "third_party/blink/renderer/core/style/style_navigation_data.h"

namespace blink {

StyleNavigationData::StyleNavigationData() : flag(NAVIGATION_TARGET_NONE) {}

StyleNavigationData::StyleNavigationData(const StyleNavigationData& o)
    : RefCounted<StyleNavigationData>(),
      id(o.id),
      target(o.target),
      flag(o.flag) {}

bool StyleNavigationData::operator==(const StyleNavigationData& o) const {
  return flag == o.flag && id == o.id && target == o.target;
}

// static
scoped_refptr<StyleNavigationData> StyleNavigationData::Create() {
  return base::AdoptRef(new StyleNavigationData);
}

scoped_refptr<StyleNavigationData> StyleNavigationData::Copy() const {
  return base::AdoptRef(new StyleNavigationData(*this));
}

}  // namespace blink
