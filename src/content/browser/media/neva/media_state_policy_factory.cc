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

#include "content/browser/media/neva/media_state_policy_factory.h"

#include "base/command_line.h"
#include "content/browser/media/neva/default_media_state_policy.h"
#include "content/browser/media/neva/limited_media_activation_count_policy.h"
#include "content/public/common/content_switches.h"

namespace content {

// static
MediaStatePolicy* MediaStatePolicyFactory::CreateMediaStatePolicy(
    MediaStatePolicy::Client* client) {
  MediaStatePolicy* policy = nullptr;

  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  if (command_line.HasSwitch(switches::kMaxActivatedMediaPlayers)) {
    policy = new LimitedMediaActivationCountPolicy(client);
  } else {
    policy = new DefaultMediaStatePolicy(client);
  }

  policy->Initialize();
  return policy;
}

}  // namespace content
