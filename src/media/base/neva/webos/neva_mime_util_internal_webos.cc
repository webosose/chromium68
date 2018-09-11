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

#include "media/base/neva/neva_mime_util_internal.h"

namespace media {

namespace internal {
NevaMimeUtil::NevaMimeUtil() {
  InitializeMimeTypeMaps();
}

NevaMimeUtil::~NevaMimeUtil() {}

void NevaMimeUtil::InitializeMimeTypeMaps() {
  AddSupportedMediaFormats();
}

void NevaMimeUtil::AddSupportedMediaFormats() {
  CodecSet webos_codecs;
  webos_codecs.insert(VALID_CODEC);

  // extended support type on tv platform
  AddContainerWithCodecs("audio/x-mpeg", webos_codecs, true);
  AddContainerWithCodecs("video/x-ms-wmv", webos_codecs, true);
  AddContainerWithCodecs("video/x-ms-asf", webos_codecs, true);
  AddContainerWithCodecs("video/x-m2ts", webos_codecs, true);
  AddContainerWithCodecs("video/m2ts", webos_codecs, true);
  // MPEG-2 TS.
  AddContainerWithCodecs("video/mp2t", webos_codecs, true);
  // hls
  AddContainerWithCodecs("application/vnd.apple.mpegurl", webos_codecs, true);
  AddContainerWithCodecs("application/vnd.apple.mpegurl.audio", webos_codecs,
                         true);
  AddContainerWithCodecs("application/mpegurl", webos_codecs, true);
  AddContainerWithCodecs("application/x-mpegurl", webos_codecs, true);
  AddContainerWithCodecs("audio/mpegurl", webos_codecs, true);
  AddContainerWithCodecs("audio/x-mpegurl", webos_codecs, true);
  // mpeg-dash
  AddContainerWithCodecs("application/dash+xml", webos_codecs, true);
  // msiis
  AddContainerWithCodecs("application/vnd.ms-sstr+xml", webos_codecs, true);
#if defined(HBBTV)
  // oipf - casd/cadd
  AddContainerWithCodecs("application/vnd.oipf.contentaccessstreaming+xml",
                         webos_codecs, true);
  AddContainerWithCodecs("application/vnd.oipf.contentaccessdownload+xml",
                         webos_codecs, true);
#endif
  // TV_Custom
  AddContainerWithCodecs("service/webos-broadcast", webos_codecs, true);
  AddContainerWithCodecs("service/webos-broadcast-cable", webos_codecs, true);
  AddContainerWithCodecs("service/webos-camera", webos_codecs, true);
  AddContainerWithCodecs("service/webos-dvr", webos_codecs, true);
  AddContainerWithCodecs("service/webos-external", webos_codecs, true);
  AddContainerWithCodecs("service/webos-photo", webos_codecs, true);
  AddContainerWithCodecs("service/webos-photo-camera", webos_codecs, true);
}

} // namespace internal
} // namespace media
