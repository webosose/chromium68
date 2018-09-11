#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright (c) 2018 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import os
import subprocess
import sys

os.path.dirname(os.path.realpath(__file__))

commit_msg = ("[skia] Fix abnormal behavior on nonclang + arm build\n"
              "\n"
              ":Release Notes:\n"
              "Applied a script patch for disabling unwanted function attribute\n"
              "\n"
              ":Detailed Notes:\n"
              "Note that this commit is applied by script execution.\n"
              "We found that real value of parameters in some functions are incorrect\n"
              "if we turn on __attribute__((pcs(\"aapcs-vfp\"))) for function. So we\n"
              "disable the attribute.\n"
              "Because this attribute is used for only arm, build for other cpu types\n"
              "are never affected by this patch.\n"
              "\n"
              ":Test Scenario:\n"
              "1. Open https://google.com\n"
              "2. box-shadow effect in search box should be rendered correctly.\n"
              "\n"
              ":QA Notes:\n"
              "\n"
              ":Issues Addressed:\n"
              "[NEVA-2635] Abnormal borders are displayed.\n"
             )

def main():
    # First check chromium version information.
    # This stage is needed for chromium65 ~ 68. So don't apply to other versions.
    chrome_version_str = subprocess.check_output("awk -F'MAJOR=' 'NR==1{print $2}' ./src/chrome/VERSION", shell=True)
    try:
        if not chrome_version_str:
            sys.stderr.write('Cannot get version information from src/chrome/VERSION.\n')
            sys.exit(1)
        if int(chrome_version_str) >= 69:
            sys.stderr.write('This stage is deprecated. Please use https://skia-review.googlesource.com/c/skia/+/143301.\n')
            sys.exit(1)
    except Exception as e:
            sys.stderr.write('Unexpected error.\n')
            sys.exit(1)

    # Find following code line and block into #if defined(__clang__)
    # #define ABI __attribute__((pcs("aapcs-vfp")))
    # ->
    # #if defined(__clang__)
    # #define ABI __attribute__((pcs("aapcs-vfp")))
    # #else
    # #define ABI
    # #endif
    subprocess.call("files=$(grep -rl '#define ABI __attribute__((pcs(\"aapcs-vfp\")))' ./src/third_party/skia/) && echo $files | xargs sed -i 's/#define ABI __attribute__((pcs(\"aapcs-vfp\")))/#if defined(__clang__)\\n#define ABI __attribute__((pcs(\"aapcs-vfp\")))\\n#else\\n#define ABI\\n#endif/g'", shell=True)
    change_result_str = subprocess.check_output("git diff --stat", shell=True, cwd="./src/third_party/skia/")

    if change_result_str:
        subprocess.call("git add .; git commit --author='JunHo Seo <junho0924.seo@lge.com>' -m '" + commit_msg + "'", shell=True, cwd="./src/third_party/skia/")

if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.stderr.write('interrupted\n')
        sys.exit(1)
else:
    print "__name__ = ", __name__
