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

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_DATA_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_DATA_H_

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_refptr.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace blink {

class StyleNavigationData : public RefCounted<StyleNavigationData> {
 public:
  enum ENavigationTarget {
    NAVIGATION_TARGET_NONE = 0,
    NAVIGATION_TARGET_CURRENT = 1,
    NAVIGATION_TARGET_NAME = 2,
    NAVIGATION_TARGET_ROOT = 4
  };

  static scoped_refptr<StyleNavigationData> Create();
  scoped_refptr<StyleNavigationData> Copy() const;

  bool operator==(const StyleNavigationData& o) const;
  bool operator!=(const StyleNavigationData& o) const { return !(*this == o); }

  WTF::String id;
  WTF::String target;
  ENavigationTarget flag;

 private:
  StyleNavigationData();
  explicit StyleNavigationData(const StyleNavigationData&);
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_STYLE_STYLE_NAVIGATION_DATA_H_
