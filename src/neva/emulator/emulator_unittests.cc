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

#include "neva/emulator/emulator_data_source.h"
#include "testing/gtest/include/gtest/gtest.h"

using namespace emulator;

namespace {

namespace emulator {

TEST(EmulatorSmokeTest, GetInstance) {
  EXPECT_NE(EmulatorDataSource::GetInstance(), nullptr);
}

TEST(EmulatorSmokeTest, GetResponseParams) {
  ResponseArgs args;

  EXPECT_TRUE(EmulatorDataSource::GetResponseParams(args, "{}"));
  EXPECT_FALSE(EmulatorDataSource::GetResponseParams(args, "{"));
}

TEST(EmulatorSmokeTest, PrepareRequestParams) {

  // Fear and loathing in C++ starts here...
  std::string escapethis = "\1 \2 \n \t < ☢";

  RequestArgumentDescription args_array[] =  {
      {"arg1", &escapethis},
      {"arg2", &escapethis}
  };

  RequestArgs args_vector = { args_array[0], args_array[1] };
  // ...ends here

  std::string prepared = EmulatorDataSource::PrepareRequestParams(args_vector);

  EXPECT_FALSE(prepared.empty());
  EXPECT_EQ(prepared, "{\\\"arg1\\\":\\\"\\\\u0001 \\\\u0002 \\\\n \\\\t \\\\u003C \xE2\x98\xA2\\\","
                       "\\\"arg2\\\":\\\"\\\\u0001 \\\\u0002 \\\\n \\\\t \\\\u003C \xE2\x98\xA2\\\"}");
}

}  // namespace emulator

}  // namespace
