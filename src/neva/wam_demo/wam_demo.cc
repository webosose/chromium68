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

#include "build/build_config.h"
#include "content/public/app/content_main.h"
#include "content/public/common/main_function_params.h"
#include "neva/wam_demo/wam_demo_main_delegate.h"
#include <memory>

int main(int argc, const char** argv) {
  base::CommandLine cmd_line(argc, argv);
  content::MainFunctionParams params(cmd_line);
  wam_demo::WamDemoMainDelegate main_delegate(params);
  content::ContentMainParams content_params(&main_delegate);
  content_params.argc = argc;
  content_params.argv = argv;

  return content::ContentMain(content_params);
}
