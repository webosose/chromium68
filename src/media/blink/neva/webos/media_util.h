// Copyright (c) 2018 LG Electronics, Inc.
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

#ifndef MEDIA_BLINK_NEVA_WEBOS_MEDIA_UTIL_H_
#define MEDIA_BLINK_NEVA_WEBOS_MEDIA_UTIL_H_

#include <string>

namespace gfx {
class Size;
}

namespace media {

struct Media3DInfo {
  std::string pattern;
  std::string type;
};

gfx::Size GetResolutionFromPAR(const std::string& par);
struct Media3DInfo GetMedia3DInfo(const std::string& media_3d_info);

}  // namespace media
#endif  // MEDIA_BLINK_NEVA_WEBOS_MEDIA_UTIL_H_
