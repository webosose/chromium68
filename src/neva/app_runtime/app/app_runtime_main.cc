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

#include "neva/app_runtime/app/app_runtime_main.h"

#include "content/public/app/content_main.h"
#include "neva/app_runtime/app/app_runtime_main_delegate.h"

int WEBVIEW_EXPORT AppRuntimeMain(int argc, const char** argv) {
  app_runtime::AppRuntimeMainDelegate app_runtime_main_delegate;
  content::ContentMainParams params(&app_runtime_main_delegate);

  params.argc = argc;
  params.argv = argv;

  return content::ContentMain(params);
}
