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

#include "media/blink/neva/webos/media_util.h"

#include "base/strings/string_number_conversions.h"
#include "ui/gfx/geometry/rect.h"

namespace media {

// Transform Pixel aspect ratio string to width/height
gfx::Size GetResolutionFromPAR(const std::string& par) {
  gfx::Size res(1, 1);

  size_t pos = par.find(":");
  if (pos == std::string::npos)
    return res;

  std::string w;
  std::string h;
  w.assign(par, 0, pos);
  h.assign(par, pos + 1, par.size() - pos - 1);

  unsigned int width;
  unsigned int height;
  if (base::StringToUint(w, &width) && base::StringToUint(h, &height))
    return gfx::Size(width, height);
  return res;
}

struct Media3DInfo GetMedia3DInfo(const std::string& media_3dinfo) {
  struct Media3DInfo res;
  res.type = "LR";

  if (media_3dinfo.find("RL") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "RL";
  } else if (media_3dinfo.find("LR") != std::string::npos) {
    res.pattern.assign(media_3dinfo, 0, media_3dinfo.size() - 3);
    res.type = "LR";
  } else if (media_3dinfo == "bottom_top") {
    res.pattern = "top_bottom";
    res.type = "RL";
  } else {
    res.pattern = media_3dinfo;
  }

  return res;
}
}
