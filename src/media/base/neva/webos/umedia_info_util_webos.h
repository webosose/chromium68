// Copyright (c) 2019 LG Electronics, Inc.
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

#ifndef MEDIA_BASE_NEVA_WEBOS_UMEDIA_INFO_UTIL_WEBOS_H_
#define MEDIA_BASE_NEVA_WEBOS_UMEDIA_INFO_UTIL_WEBOS_H_

#include <uMediaClient.h>

#include "third_party/jsoncpp/source/include/json/json.h"

#if UMS_INTERNAL_API_VERSION == 2
#include "media/base/neva/webos/umedia_info_util_webos_gmp.h"
#endif

namespace media {

enum class PlaybackNotification {
  NotifyPreloadCompleted,
  NotifyLoadCompleted,
  NotifyUnloadCompleted,
  NotifyEndOfStreamForward,
  NotifyEndOfStreamBackward,
  NotifyPaused,
  NotifyPlaying,
  NotifySeekDone,
};

std::string PlaybackNotificationToJson(const std::string&,
                                       const PlaybackNotification);
std::string ExternalSubtitleTrackInfoToJson(
    const std::string&,
    const struct uMediaServer::external_subtitle_track_info_t&);
std::string ErrorInfoToJson(const std::string&,
                            int64_t errorCode,
                            const std::string& errorText);
std::string UserDefinedInfoToJson(const std::string&,
                                  const std::string& message);
std::string MasterClockInfoToJson(
    const std::string&,
    const struct uMediaServer::master_clock_info_t&);
std::string SlaveClockInfoToJson(
    const std::string&,
    const struct uMediaServer::slave_clock_info_t&);

}  // namespace media

#endif  // MEDIA_BASE_NEVA_WEBOS_UMEDIA_INFO_UTIL_WEBOS_H_
