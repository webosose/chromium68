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

#include "base/command_line.h"
#include "base/run_loop.h"
#include "content/public/common/content_switches.h"
#include "neva/app_runtime/common/app_runtime_user_agent.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace app_runtime {

TEST(AppRuntimeTest, AddSuffixToContentUserAgent) {
  base::CommandLine::ForCurrentProcess()->AppendSwitchASCII(
      switches::kUserAgentSuffix, "Smart Refrigerator");

  std::string expected_result =
      "Mozilla/5.0 (webOS; Linux/Smart Refrigerator) AppleWebKit/537.36 "
      "(KHTML, like Gecko) Chrome/68.0.3440.106 Safari/537.36";

  EXPECT_EQ(expected_result, GetUserAgent());
}

}  // namespace app_runtime
