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

#ifndef MEDIA_BLINK_NEVA_MEDIAPLAYERNEVA_FACTORY_H_
#define MEDIA_BLINK_NEVA_MEDIAPLAYERNEVA_FACTORY_H_

#include <string>
#include <memory>
#include "base/memory/ref_counted.h"
#include "base/single_thread_task_runner.h"
#include "media/blink/neva/media_player_neva_interface.h"

namespace media {

class MediaPlayerNevaFactory {
    public:
    static bool CanSupportMediaType(const std::string& mime_type);
    static MediaPlayerNeva* CreateMediaPlayerNeva(MediaPlayerNevaClient*,
            const std::string&,
            const scoped_refptr<base::SingleThreadTaskRunner>&,
            const std::string&);
};

} // media namespace

#endif // MEDIA_BLINK_NEVA_MEDIAPLAYERNEVA_FACTORY_H_
